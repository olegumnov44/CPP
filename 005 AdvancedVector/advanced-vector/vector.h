#include <cstdlib>
#include <memory>
#include <utility>
#include <cassert>
#include <new>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t new_size) {
        buffer_ = Allocate(new_size);
        capacity_ = new_size;
    }

    RawMemory(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(other.buffer_)
        , capacity_(other.capacity_)
    {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory& operator=(const RawMemory&) = delete;

    RawMemory& operator=(RawMemory&& other) noexcept {
        Swap(other);
        return *this;
    }

    T& operator[](const size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    const T& operator[](const size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T* operator+(size_t add) noexcept {
        assert(add <= capacity_);
        return buffer_ + add;
    }

    const T* operator+(size_t add) const noexcept {
        return const_cast<RawMemory&>(*this) + add;
    }

    void Swap(RawMemory<T>& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetBuffer() const noexcept {
        return buffer_;
    }
    T* GetBuffer() noexcept {
        return buffer_;
    }
    size_t GetCapacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>((operator new(sizeof(T) * n))) : nullptr;
    }
    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buffer) {
        operator delete(buffer);
    }
    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    Vector(size_t new_size): data_(new_size)
    {
        size_ = new_size;
        std::uninitialized_value_construct_n(data_.GetBuffer(), new_size);
    }

    Vector(const Vector& other): data_(other.size_)
    {
        size_ = other.size_;
        std::uninitialized_copy_n(other.data_.GetBuffer(), other.size_, data_.GetBuffer());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.Size())
    {
        other.size_ = 0;
    }

    ~Vector<T>()
    {
        std::destroy_n(data_.GetBuffer(), size_);
    }

    Vector<T>& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.GetCapacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                const size_t copy_count = std::min(size_, rhs.size_);
                std::copy_n(rhs.data_.GetBuffer(), copy_count, data_.GetBuffer());
                if (size_ < rhs.size_) {
                    std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
                } else if (size_ > rhs.size_) {
                    std::destroy_n(data_ + rhs.size_, size_ - rhs.size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector<T>& operator=(Vector&& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
        return *this;
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    void Reserve(size_t new_size) {
        if (new_size > data_.GetCapacity()) {
            RawMemory<T> data2(new_size);
            UninitializedMoveNIfNoexcept(data_.GetBuffer(), size_, data2.GetBuffer());
            std::destroy_n(data_.GetBuffer(), size_);
            data_.Swap(data2);
        }
    }

    void Resize(size_t new_size) {
        Reserve(new_size);
        if (size_ < new_size) {
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        } else if (size_ > new_size) {
            std::destroy_n(data_ + new_size, size_ - new_size);
        }
        size_ = new_size;
    }

    void Clear() {
        std::destroy_n(data_.GetBuffer(), size_);
        size_ = 0;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.GetCapacity();
    }

    void PushBack(const T& elem) {
        EmplaceBack(elem);
    }
    void PushBack(T&& elem) {
        EmplaceBack(std::move(elem));
    }
    void PopBack() {
        assert(size_ > 0);
        --size_;
        std::destroy_at(data_ + size_);
    }
    template<typename... Args>
    T& EmplaceBack(Args&& ... args) {
        if (size_ == Capacity()) {
            // Требуется реаллокация памяти
            // Аргументы args потенциально могут принадлежать объекту, находящемуся в текущем
            // векторе, поэтому нельзя уничтожать data_ до завершения операции вставки
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            // Сначала конструируем вставляемый элемент (может выбросить исключение)
            new (new_data + size_) T(std::forward<Args>(args)...);
            try {
                // Переносим либо копируем оставшиеся элементы
                UninitializedMoveNIfNoexcept(data_.GetBuffer(), size_, new_data.GetBuffer());
            } catch (...) {
                // Вставленный элемент нужно разрушить
                std::destroy_at(new_data + size_);
                throw;
            }
            std::destroy_n(data_.GetBuffer(), size_);
            data_.Swap(new_data);
        } else {
            // Реаллокация не требуется. Просто конструируем новый элемент
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        return data_[size_++];
    }

    iterator Insert(const_iterator pos, const T& value) {
            return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t offset = pos - data_.GetBuffer();
        T* insert_pos = data_ + offset;
        if (size_ == Capacity()) {
            InsertWithRealloc(insert_pos, std::forward<Args>(args)...);
        } else {
            if (offset == size_) {
                new (insert_pos) T(std::forward<Args>(args)...);
                ++size_;
            } else {
                T tmp_copy(std::forward<Args>(args)...);
                InsertWithoutRealloc(insert_pos, std::move(tmp_copy));
            }
        }
        return data_ + offset;
    }

    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t offset = pos - cbegin();
        if (offset + 1 < size_) {
            std::move(data_ + offset + 1, data_ + size_, data_ + offset);
        }
        PopBack();
        return begin() + offset;
    }

    iterator begin() noexcept {
        return data_.GetBuffer();
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetBuffer();
    }
    const_iterator end() const noexcept {
        return data_ + size_;
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

private:
    template <typename Arg>
    void InsertWithoutRealloc(T* pos, Arg&& arg) {
        assert(Capacity() > Size());
        new (data_ + size_) T(std::move(data_[size_ - 1]));
        ++size_;
        std::move_backward(pos, data_ + size_ - 2, data_ + size_ - 1);
        *pos = std::forward<Arg>(arg);
    }
    template <typename... Args>
    void InsertWithRealloc(T* pos, Args&&... args) {
        assert(Capacity() == Size());
        // Требуется реаллокация памяти
        // Аргументы args потенциально могут принадлежать объекту, находящемуся в текущем
        // векторе, поэтому нельзя уничтожать data_ до завершения операции вставки
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        const size_t offset = pos - data_.GetBuffer();
        T* const old_start = data_.GetBuffer();
        T* const new_start = new_data.GetBuffer();
        T* new_finish = new_start;
        T* const new_pos = new_start + offset;
        try {
            new (new_pos) T(std::forward<Args>(args)...);
            // обнуляем new_finish, чтобы в блоке catch разрушить только объект в позиции new_pos
            new_finish = nullptr;
            // Перемещаем элементы, предшествующие позиции вставки
            new_finish = UninitializedMoveNIfNoexcept(old_start, offset, new_start) + 1;
            // Перемещаем элементы, следующие за позицией вставки
            new_finish = UninitializedMoveNIfNoexcept(pos, size_ - offset, new_finish);
        } catch (...) {
            if (!new_finish) {
                std::destroy_at(new_pos);
            } else {
                std::destroy(new_data.GetBuffer(), new_finish);
            }
            throw;
        }
        std::destroy_n(old_start, size_);
        data_.Swap(new_data);
        ++size_;
    }

    static T* UninitializedMoveNIfNoexcept(T* from, size_t new_size, T* to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            return std::uninitialized_move_n(from, new_size, to).second;
        } else {
            return std::uninitialized_copy_n(from, new_size, to);
        }
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};
