#pragma once

#include <cassert>
#include <iostream>
#include <cstdlib>
#include <algorithm>

template <typename Type>
class ArrayPtr {
public:
    // Инициализирует ArrayPtr нулевым указателем
    ArrayPtr() = default;

    // Создаёт в куче массив из size элементов типа Type.
    // Если size == 0, поле raw_ptr_ должно быть равно nullptr
    explicit ArrayPtr(size_t size) {
        if (size > 0) {
            raw_ptr_ = std::move(new Type[size]);
            size_ = size;
        } else {
            raw_ptr_ = nullptr;
        }
    }

    // Конструктор из сырого указателя, хранящего адрес массива в куче либо nullptr
    explicit ArrayPtr(Type* raw_ptr) noexcept {

        raw_ptr_ = std::move(raw_ptr);
    }

    // Запрещаем копирование
    ArrayPtr(const ArrayPtr& other) = delete;

    // Запрещаем присваивание копированием
    ArrayPtr& operator=(const ArrayPtr& rhs) = delete;

    // Разрешаем перемещение
    //ArrayPtr(ArrayPtr&&) = default;

    // Разрешаем перемещение
    ArrayPtr(ArrayPtr&& other) { *this = std::move(other); }

    // Разрешаем присваивание перемещением
    //ArrayPtr& operator=(ArrayPtr&&) = default;

    // Оператор присваивания перемещением
    ArrayPtr& operator=(ArrayPtr&& rhs) {
        assert(this != &rhs);
        if (raw_ptr_ != nullptr) {
            delete[] raw_ptr_;
        }
        size_ = rhs.GetSize();
        raw_ptr_ = new Type[size_]{};
        std::move(std::make_move_iterator(&rhs.raw_ptr_[0]),
                  std::make_move_iterator(&rhs.raw_ptr_[size_]), raw_ptr_);

        return *this;
    }

    ~ArrayPtr() {
        Delete();
    }

    // Прекращает владением массивом в памяти, возвращает значение адреса массива
    // После вызова метода указатель на массив должен обнулиться
    [[nodiscard]] Type* Release() noexcept {
        Type* t = raw_ptr_;
        raw_ptr_ = nullptr;
        return t;
    }

    // Возвращает ссылку на элемент массива с индексом index
    Type& operator[](size_t index) noexcept {
        return raw_ptr_[index];
    }

    // Возвращает константную ссылку на элемент массива с индексом index
    const Type& operator[](size_t index) const noexcept {
        return raw_ptr_[index];
    }

    // Возвращает true, если указатель ненулевой, и false в противном случае
    explicit operator bool() const {
        return raw_ptr_ == nullptr ? true : false;
    }

    // Возвращает значение сырого указателя, хранящего адрес начала массива
    Type* Get() const noexcept {
        return &raw_ptr_[0];
    }

    // Обменивается значением указателя на массив с объектом other
    void swap(ArrayPtr& other) noexcept {
        assert(this->raw_ptr_ != other.raw_ptr_);
        std::swap(raw_ptr_, other.raw_ptr_);
    }

    // Обменивается значением указателя на массив с объектом other
    void swap(ArrayPtr&& other) noexcept {
        assert(*this != other.raw_ptr_);
        std::exchange(raw_ptr_, other.raw_ptr_);
    }

    // Удаление массива указателей
    void Delete() {
        delete[] raw_ptr_;
        raw_ptr_ = nullptr;
    }

private:
    Type* raw_ptr_ = nullptr;
    size_t size_ = 0;
};
