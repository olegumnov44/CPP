#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include "array_ptr.h"
//#include "my_assert.h"


//Класс-обёртка для создания вектора по функции (не методу!) Reserve
class ReserveProxyObj {
public:

    ReserveProxyObj() noexcept = default;

    ReserveProxyObj (size_t capacity_to_reserve)
        : capacity_to_reserve_(capacity_to_reserve)
    {}

    size_t GetValue() { return capacity_to_reserve_; }

private:
    size_t capacity_to_reserve_ = 0;
};

// Функция Reserve для запуска класса-обёртки
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    // Создаёт пустой вектор
    SimpleVector() noexcept
        : size_(0)
        , capacity_(0)
        , vector_{ArrayPtr<Type>{}}
    {}

    // Создаёт пустой вектор c заданной ёмкостью
    SimpleVector(ReserveProxyObj obj)
        : size_(0)
        , capacity_(obj.GetValue())
        , vector_{size_}
    {}

    // Создаёт вектор из size элементов, инициализированных значением value (или по умолчанию)
    SimpleVector(size_t size, const Type& value = Type())
        : size_(size)
        , capacity_(size)
        , vector_{size_}
    {
        std::fill(vector_.Get(), vector_.Get() + size_, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init)
        : size_(init.size())
        , capacity_(init.size())
        , vector_{size_}
    {
        std::copy(init.begin(), init.end(), vector_.Get());
    }

    // Создаёт копию другого вектора (конструктор копирования)
    SimpleVector(const SimpleVector& other)
        : size_(other.size_)
        , capacity_(other.capacity_)
        , vector_{size_}
    {
        //assert((*this != other) && "Error: Himself's copy");
        std::copy(other.begin(), other.end(), vector_.Get());
    }

    // ПЕРЕМЕЩЕНИЕ
    // Перемещает вектор в другой вектор (конструктор перемещения)
    SimpleVector(SimpleVector&& other)
        : size_(other.size_)
        , capacity_(other.capacity_)
        , vector_{size_}
    {
        std::move(other.begin(), other.end(), vector_.Get());

        other.Clear();
        other.capacity_ = 0;
        other.vector_.Delete();
    }

    // Оператор присваивания копированием
    SimpleVector& operator=(const SimpleVector& rhs) {
        SimpleVector tmp{rhs};
        this->swap(tmp);   // copy&swap
        tmp.Clear();
        tmp.capacity_ = 0;
        tmp.vector_.Delete();
        return *this;
    }

    // ПЕРЕМЕЩЕНИЕ
    // Опереатор присваивания перемещением
    SimpleVector& operator=(SimpleVector&& rhs) {
        this->swap(rhs);
        return *this;
    }

    // Деструктор
    ~SimpleVector() { Clear(); }

    // Возвращает количество элементов в массиве
    std::size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    std::size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return GetSize() == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](std::size_t index) noexcept {
        assert((index <= size_) && "Error: Out of range!");
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](std::size_t index) const noexcept {
        assert((index <= size_) && "Error: Out of range!");
        return const_cast<Type&>(vector_[index]);
    }

    // Возвращает ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Error: Out of range!");
        }
        return vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Error: Out of range!");
        }
        return const_cast<Type&>(vector_[index]);
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(vector_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return ConstIterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return ConstIterator(vector_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ConstIterator(vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator(vector_.Get() + size_);
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_) {
            size_ = new_size;
         }
            else if (new_size < capacity_) {
                //std::fill(begin() + size_, begin() + new_size, Type()); //не проходит тренажёр
                for (auto it = begin() + size_; it != begin() + new_size; ++it) { *it = std::move(Type()); }
                size_ = new_size;
                }
                else if (new_size >= capacity_) {
                        size_t new_capacity = 2*new_size;
                        try {
                            ArrayPtr<Type> tmp{new_size};
                            std::move(begin(), begin() + size_, tmp.Get());
                            vector_.swap(tmp);
                            tmp.Delete();
                        }
                        catch (std::bad_alloc&) {
                            std::cerr << "Error: Bad allocation!" << std::endl;
                            throw std::bad_alloc();
                        }
                        size_ = new_size;
                        capacity_ = new_capacity;
                    }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ == 0) {
            capacity_ = 1;
            ArrayPtr<Type> tmp{capacity_};
            vector_.swap(tmp);
            tmp.Delete();
        }
        if (size_ < capacity_) {
            vector_[size_] = item;
            ++size_;
        }
        else {
            ArrayPtr<Type> tmp{capacity_ * 2};
            std::copy(begin(), end(), tmp.Get());
            tmp[size_] = item;
            vector_.swap(tmp);
            ++size_;
            capacity_ *= 2;
            tmp.Delete();
        }
    }

    // ПЕРЕМЕЩЕНИЕ
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(Type&& item) {
        if (capacity_ == 0) {
            capacity_ = 1;
            ArrayPtr<Type> tmp{capacity_};
            vector_.swap(tmp);
            tmp.Delete();
        }
        if (size_ < capacity_) {
            vector_[size_] = std::move(item);
            ++size_;
        }
        else {
            ArrayPtr<Type> tmp{capacity_ * 2};
            std::move(begin(), end(), tmp.Get());
            tmp[size_] = std::move(item);
            vector_.swap(tmp);
            ++size_;
            capacity_ *= 2;
            tmp.Delete();
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type& value) {
        assert( (pos >= begin() && pos <= end()) && "Error: Out of range!" );
        Iterator it_pos = const_cast<Iterator>(pos);
        if (size_ < capacity_) {
            std::copy_backward(it_pos, end(), end() + 1);
            *it_pos = value;
            ++size_;
            return it_pos;
        }
        size_t new_capacity = (capacity_ > 0 ? 2*capacity_: 1);
        auto distance = std::distance(vector_.Get(), it_pos);
        ArrayPtr<Type> tmp{new_capacity};
        if (distance == 0) {
            std::copy(vector_.Get(), vector_.Get() + size_, tmp.Get() + 1);
        } else {
            std::copy(vector_.Get(), it_pos, tmp.Get());
            std::copy(it_pos, vector_.Get() + size_, tmp.Get() + distance + 1);
        }
        *(tmp.Get() + distance) = value;
        vector_.swap(tmp);
        ++size_;
        capacity_ = new_capacity;
        tmp.Delete();
        return Iterator{vector_.Get() + distance};
    }

    // ПЕРЕМЕЩЕНИЕ
    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        assert( (pos >= begin() && pos <= end()) && "Error: Out of range!" );
        Iterator it_pos = const_cast<Iterator>(pos);

        if (size_ < capacity_) {
            std::move_backward(it_pos, end(), end() + 1);
            *it_pos = std::move(value);
            ++size_;
            return it_pos;
        }

        size_t new_capacity = (capacity_ > 0 ? 2*capacity_ : 1);
        auto distance = std::distance(begin(), it_pos);
        ArrayPtr<Type> tmp{new_capacity};
        if (distance == 0) {
            std::move(begin(), end(), tmp.Get() + 1);
        } else {
            std::move(begin(), it_pos, tmp.Get());
            std::move(it_pos, end(), tmp.Get() + distance + 1);
        }
        *(tmp.Get() + distance) = std::move(value);
        vector_.swap(tmp);
        ++size_;
        capacity_ = new_capacity;
        tmp.Delete();
        return Iterator{begin() + distance};
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty() && "Error: Vector is empty!");
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    // Возвращает итератор на, следующий после удалённого, элемент
    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty() && "Error: Vector is empty!");
        assert( (pos >= begin() && pos <= end()) && "Error: Out of range!" );
        Iterator it_pos = const_cast<Iterator>(pos);

        //std::move_backward(it_pos + 1, end(), end() - 1);

        for (auto it = it_pos; it != end(); ++it) { *(it) = std::move(*(it + 1)); }

        --size_;
        return Iterator{it_pos};
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        //assert((*this != other) && "Error: Himself's swap");
        vector_.swap(other.vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // ПЕРЕМЕЩЕНИЕ
    // Обменивает значение с другим вектором
    void swap(SimpleVector&& other) noexcept {
        //assert((*this != other) && "Error: Himself's swap");
        vector_.swap(other.vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Метод резервирования ёмкости вектора
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            try {
                ArrayPtr<Type> tmp{new_capacity};
                std::copy(begin(), end(), tmp.Get());
                std::fill(tmp.Get() + size_, tmp.Get() + new_capacity, Type());
                vector_.swap(tmp);
                tmp.Delete();
            }
            catch (std::bad_alloc&) {
                std::cerr << "Error: Bad allocation!" << std::endl;
                throw std::bad_alloc();
            }
            capacity_ = new_capacity;
        }
    }

private:
    size_t size_;
    size_t capacity_;
    ArrayPtr<Type> vector_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs < rhs) || (lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs <= rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
