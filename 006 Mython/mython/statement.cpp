#include <iostream>
#include <sstream>

#include "statement.h"

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;
namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
const string NONE = "None"s;
template <typename T>
bool TryAddValues(const ObjectHolder& left, const ObjectHolder& right, ObjectHolder& result) {
    const auto* l = left.TryAs<T>();
    const auto* r = right.TryAs<T>();
    if (l != nullptr && r != nullptr) {
        result = ObjectHolder::Own(T(l->GetValue() + r->GetValue()));
        return true;
    }
    return false;
}
bool TryAddInstances(ObjectHolder& left, ObjectHolder& right, ObjectHolder& result,
                     Context& context) {
    if (auto* l = left.TryAs<runtime::ClassInstance>();
        l != nullptr && l->HasMethod(ADD_METHOD, 1)) {
        result = l->Call(ADD_METHOD, {right}, context);
        return true;
    }
    return false;
}
}  // namespace
ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    return closure[var_name_] = right_value_->Execute(closure, context);
}
Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_name_(std::move(var))
    , right_value_(std::move(rv)) {
}
VariableValue::VariableValue(const std::string& var_name)
    : dotted_ids_(1, var_name) {
}
VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    : dotted_ids_(std::move(dotted_ids))  //
{
    if (this->dotted_ids_.empty()) {
        throw std::runtime_error("You can't create VariableValue with empty dotted_ids"s);
    }
}
ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    auto* cur_closure = &closure;
    for (size_t i = 0; i + 1 < dotted_ids_.size(); ++i) {
        auto it = cur_closure->find(dotted_ids_[i]);
        if (it == cur_closure->end()) {
            throw std::runtime_error("Name "s + dotted_ids_[i] + " not found in the scope"s);
        }
        if (auto* p = it->second.TryAs<runtime::ClassInstance>(); p) {
            cur_closure = &p->Fields();
        } else {
            throw std::runtime_error(dotted_ids_[i]
                                     + " is not an object, can't access its fields"s);
        }
    }
    if (auto it = cur_closure->find(dotted_ids_.back()); it != cur_closure->end()) {
        return it->second;
    }
    throw std::runtime_error("Variable "s + dotted_ids_.back() + " not found in closure"s);
}
unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
}
Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(std::move(argument));
}
Print::Print(vector<unique_ptr<Statement>> args)
    : args_(std::move(args)) {
}
ObjectHolder Print::Execute(Closure& closure, Context& context) {
    bool first = true;
    auto& output = context.GetOutputStream();
    for (const auto& statement : args_) {
        if (!first) {
            output << ' ';
        }
        first = false;
        if (ObjectHolder result = statement->Execute(closure, context)) {
            result->Print(output, context);
        } else {
            output << NONE;
        }
    }
    output << '\n';
    return ObjectHolder::None();
}
MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
    : object_(std::move(object))
    , method_(std::move(method))
    , args_(std::move(args)) {
}
ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    vector<ObjectHolder> actual_args;
    for (auto& stmt : args_) {
        actual_args.push_back(stmt->Execute(closure, context));
    }
    ObjectHolder callee = object_->Execute(closure, context);
    if (auto* instance = callee.TryAs<runtime::ClassInstance>(); instance != nullptr) {
        return instance->Call(method_, actual_args, context);
    }
    throw std::runtime_error("Trying to call method "s + method_
                             + " on object which is not a class instance"s);
}
ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    std::ostringstream os;
    if (ObjectHolder arg_value = argument_->Execute(closure, context); arg_value) {
        arg_value->Print(os, context);
    } else {
        os << NONE;
    }
    return ObjectHolder::Own(runtime::String(os.str()));
}
ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto left = lhs_->Execute(closure, context);
    auto right = rhs_->Execute(closure, context);
    ObjectHolder result;
    bool success = TryAddValues<runtime::Number>(left, right, result);
    success = success || TryAddValues<runtime::String>(left, right, result);
    success = success || TryAddInstances(left, right, result, context);
    if (success) {
        return result;
    }
    throw std::runtime_error("Addition isn't supported for these operands"s);
}
ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto left = lhs_->Execute(closure, context);
    auto right = rhs_->Execute(closure, context);
    auto* left_number = left.TryAs<runtime::Number>();
    auto* right_number = right.TryAs<runtime::Number>();
    if (left_number != nullptr && right_number != nullptr) {
        return ObjectHolder::Own(
            runtime::Number(left_number->GetValue() - right_number->GetValue()));
    }
    throw std::runtime_error("Subtraction is supported only for integers"s);
}
ObjectHolder Mult::Execute(runtime::Closure& closure, Context& context) {
    auto left = lhs_->Execute(closure, context);
    auto right = rhs_->Execute(closure, context);
    auto* left_number = left.TryAs<runtime::Number>();
    auto* right_number = right.TryAs<runtime::Number>();
    if (left_number != nullptr && right_number != nullptr) {
        return ObjectHolder::Own(
            runtime::Number(left_number->GetValue() * right_number->GetValue()));
    }
    throw std::runtime_error("Multiplication is supported only for integers"s);
}
ObjectHolder Div::Execute(runtime::Closure& closure, Context& context) {
    auto left = lhs_->Execute(closure, context);
    auto right = rhs_->Execute(closure, context);
    auto* left_number = left.TryAs<runtime::Number>();
    auto* right_number = right.TryAs<runtime::Number>();
    if (left_number == nullptr || right_number == nullptr) {
        throw std::runtime_error("Division is supported only for integers"s);
    }
    if (right_number->GetValue() == 0) {
        throw std::runtime_error("Division by zero"s);
    }
    return ObjectHolder::Own(runtime::Number(left_number->GetValue() / right_number->GetValue()));
}
ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (auto& stmt : statements_) {
        stmt->Execute(closure, context);
    }
    return ObjectHolder::None();
}
ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure, context);
}
ClassDefinition::ClassDefinition(ObjectHolder cls)
    : class_(std::move(cls))
    , name_(dynamic_cast<const runtime::Class&>(*class_).GetName()) {
}
ObjectHolder ClassDefinition::Execute(runtime::Closure& closure,
                                      [[maybe_unused]] Context& context) {
    closure[name_] = class_;
    return ObjectHolder::None();
}
FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
    : object_(std::move(object))
    , field_name_(std::move(field_name))
    , right_value_(std::move(rv)) {
}
ObjectHolder FieldAssignment::Execute(runtime::Closure& closure, Context& context) {
    auto instance = object_.Execute(closure, context);
    if (auto* p = instance.TryAs<runtime::ClassInstance>(); p != nullptr) {
        return p->Fields()[field_name_] = right_value_->Execute(closure, context);
    }
    throw std::runtime_error("Cannot assign to the field "s + field_name_ + " of not an object"s);
}
IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
    : condition_(std::move(condition))
    , if_body_(std::move(if_body))
    , else_body_(std::move(else_body)) {
}
ObjectHolder IfElse::Execute(runtime::Closure& closure, Context& context) {
    auto value_ = condition_->Execute(closure, context);
    if (IsTrue(value_)) {
        if_body_->Execute(closure, context);
    } else if (else_body_) {
        else_body_->Execute(closure, context);
    }
    return ObjectHolder::None();
}
ObjectHolder Or::Execute(runtime::Closure& closure, Context& context) {
    if (IsTrue(lhs_->Execute(closure, context)) || IsTrue(rhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}
ObjectHolder And::Execute(runtime::Closure& closure, Context& context) {
    if (IsTrue(lhs_->Execute(closure, context)) && IsTrue(rhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}
ObjectHolder Not::Execute(runtime::Closure& closure, Context& context) {
    return ObjectHolder::Own(runtime::Bool(!IsTrue(argument_->Execute(closure, context))));
}
Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs))
    , comparator_(std::move(cmp)) {
}
ObjectHolder Comparison::Execute(runtime::Closure& closure, Context& context) {
    auto lhs_value = lhs_->Execute(closure, context);
    auto rhs_value = rhs_->Execute(closure, context);
    return ObjectHolder::Own(runtime::Bool(comparator_(lhs_value, rhs_value, context)));
}
NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    : class_(class_)
    , args_(std::move(args)) {
}
NewInstance::NewInstance(const runtime::Class& class_)
    : NewInstance(class_, {}) {
}
ObjectHolder NewInstance::Execute(runtime::Closure& closure, Context& context) {
    auto instance = ObjectHolder::Own(runtime::ClassInstance(class_));
    if (const auto* m = class_.GetMethod(INIT_METHOD); m != nullptr) {
        vector<ObjectHolder> actual_args;
        for (auto& stmt : args_) {
            actual_args.push_back(stmt->Execute(closure, context));
        }
        instance.TryAs<runtime::ClassInstance>()->Call(INIT_METHOD, actual_args, context);
    }
    return instance;
}
MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
    : body_(std::move(body)) {
}
ObjectHolder MethodBody::Execute(runtime::Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
    } catch (ObjectHolder& return_value) {
        return std::move(return_value);
    }
    // Метод завершился без return, возвращаем None
    return {};
}
}  // namespace ast
