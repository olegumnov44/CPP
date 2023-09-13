#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include "test_runner_p.h"

#include <iostream>

using namespace std;

/*
--Задание
Разработайте интерпретатор языка Mython, который считывает из stdin программу на языке Mython
и выводит в stdout результат выполнения всех команд print.
В заготовке кода вам даны следующие файлы:

    parse.h и parse.cpp — синтаксический анализатор (парсер) языка Mython.
    statement.h и statement.cpp — объявления классов узлов абстрактного синтаксического дерева (AST).
        Парсер использует эти классы в процессе построения AST.
    runtime.h и runtime.cpp — классы, управляющие состоянием интерпретатора. Используйте решение из предыдущего урока.
    lexer.h и lexer.cpp — лексический анализатор. Используйте своё решение из предыдущих уроков.
    main.cpp — главный файл программы.
    statement_test.cpp, parse_test.cpp, runtime_test.cpp, lexer_test_open.cpp —
        файлы юнит-тестов для компонентов интерпретатора. В файле test_runner.h — классы и макросы,
        необходимые для работы тестов.

--Что отправлять на проверку
Отправьте на проверку код модулей интерпретатора и основной программы.
Не изменяйте сигнатуру публичных методов классов и функций, объявленных в заголовочных файлах,
иначе программа не будет принята.
--Как будет тестироваться ваша программа
Тренажёр проверит работу классов и функций, используя набор тестов, подобным представленном в заготовке.
При этом функция main будет заменена на версию из тренажёра.
После этого тренажёр проверит работу интерпретатора в целом.
Для этого он передаст ей в stdin код программы на языке Mython и сравнит вывод программы с эталонными значениями.
Гарантируется, что на вход программы будут подаваться только синтаксически корректные Mython-программы.
--Подсказка
При вызове конструировании экземпляра пользовательского класса не забудьте вызвать у него метод __init__.
Чтобы реализовать в интерпретаторе инструкцию return, выбросьте исключение в методе Return::Execute
и поймайте его в MethodBody::Execute. Брошенное исключение должно нести информацию о возвращаемом значении метода.
Если метод завершился без выбрасывания исключения, результат его работы — значение None.
*/

namespace parse {
void RunOpenLexerTests(TestRunner& tr);
}  // namespace parse

namespace ast {
void RunUnitTests(TestRunner& tr);
}
namespace runtime {
void RunObjectHolderTests(TestRunner& tr);
void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

void TestParseProgram(TestRunner& tr);

namespace {

void RunMythonProgram(istream& input, ostream& output) {
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context{output};
    runtime::Closure closure;
    program->Execute(closure, context);
}

void TestSimplePrints() {
    istringstream input(R"(
print 57
print 10, 24, -8
print 'hello'
print "world"
print True, False
print
print None
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\n10 24 -8\nhello\nworld\nTrue False\n\nNone\n");
}

void TestAssignments() {
    istringstream input(R"(
x = 57
print x
x = 'C++ black belt'
print x
y = False
x = y
print x
x = None
print x, y
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\nC++ black belt\nFalse\nNone False\n");
}

void TestArithmetics() {
    istringstream input("print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
}

void TestVariablesArePointers() {
    istringstream input(R"(
class Counter:
  def __init__():
    self.value = 0

  def add():
    self.value = self.value + 1

class Dummy:
  def do_add(counter):
    counter.add()

x = Counter()
y = x

x.add()
y.add()

print x.value

d = Dummy()
d.do_add(x)

print y.value
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "2\n3\n");
}

void TestAll() {
    TestRunner tr;
    parse::RunOpenLexerTests(tr);
    runtime::RunObjectHolderTests(tr);
    runtime::RunObjectsTests(tr);
    ast::RunUnitTests(tr);
    TestParseProgram(tr);

    RUN_TEST(tr, TestSimplePrints);
    RUN_TEST(tr, TestAssignments);
    RUN_TEST(tr, TestArithmetics);
    RUN_TEST(tr, TestVariablesArePointers);
}

}  // namespace

int main() {
    try {
        TestAll();

        RunMythonProgram(cin, cout);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
