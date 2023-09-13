
#include "tests.h"


/*
--Задание 1
Реализуйте в классе Vector метод EmplaceBack, добавляющий новый элемент в конец вектора.
Созданный объект должен быть сконструирован с использованием аргументов метода EmplaceBack.

template <typename T>
class Vector {
public:
    template <typename... Args>
    T& EmplaceBack(Args&&... args);
};

Метод EmplaceBack должен предоставлять строгую гарантию безопасности исключений, когда выполняется любое из условий:

    move-конструктор у типа T объявлен как noexcept;
    тип T имеет публичный конструктор копирования.

Если у типа T нет конструктора копирования и move-конструктор может выбрасывать исключения,
метод EmplaceBack должен предоставлять базовую гарантию безопасности исключений.
Сложность метода EmplaceBack должна быть амортизированной константой.
Работу вам упростит набор тестов:

    tests.cpp

--Ограничения
Не меняйте сигнатуру публичных методов класса Vector. Иначе ваше решение может быть отклонено.
Класс Vector не должен ничего выводить в потоки stdout и stderr. В противном случае ваше решение может быть отклонено.
--Как будет тестироваться ваша программа
Тренажёр проверит работу класса Vector, включая разработанный в этом задании метод EmplaceBack.
Код скомпилируется со включенными UB и Address санитайзерами, чтобы выявить потенциальные проблемы
при работе с указателями и динамической памятью.
Функция main будет заменена на версию из тренажёра.
--Подсказка
    Когда работа над методом EmplaceBack завершится, его можно будет использовать для реализации обоих методов PushBack.
    Чтобы передать параметры конструктору элементов вектора, используйте forwarding-ссылки и std::forward.

--Задание 2
Это последняя часть итогового проекта тринадцатого спринта. Сохраните решение на GitHub, чтобы сдать его на ревью.
Реализуйте методы Insert, Emplace и Erase в классе Vector, а также методы begin, cbegin, end и cend для получения итераторов
на начало и конец вектора.
Сигнатура методов:

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos) / noexcept(std::is_nothrow_move_assignable_v<T>) /;
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);
    ...
};

Методы Emplace и Erase, выполняя реаллокацию, должны перемещать элементы, а не копировать, если действительно любое из условий:

    тип T имеет noexcept-конструктор перемещения;
    тип T не имеет конструктора копирования.

Если при вставке происходит реаллокация, ожидается ровно Size() перемещений или копирований существующих элементов.
Сам вставляемый элемент должен копироваться либо перемещаться в зависимости от версии метода Insert.
Вызов Insert и Emplace, не требующий реаллокации, должен перемещать ровно end()-pos существующих элементов плюс одно перемещение
элемента из временного объекта в вектор.
Методы Insert и Emplace при вставке элемента в конец вектора должны обеспечивать строгую гарантию безопасности исключений,
когда выполняется любое из условий:

    шаблонный параметр T имеет конструктор копирования;
    шаблонный параметр T имеет noexcept-конструктор перемещения.

Если ни одно из этих условий не выполняется либо элемент вставляется в середину или начало вектора,
методы Insert и Emplace должны обеспечивать базовую гарантию безопасности исключений.
Метод Erase должен вызывать деструктор ровно одного элемента, а также вызывать оператор присваивания столько раз,
сколько элементов находится в векторе следом за удаляемым элементом. Итератор pos, который задаёт позицию удаляемого элемента,
должен указывать на существующий элемент вектора. Передача в метод Erase итератора end(), невалидного итератора или итератора,
полученного у другого вектора, приводит к неопределённому поведению.
Работу вам упростит набор тестов:

    tests.cpp

--Ограничения
Не меняйте сигнатуру публичных методов класса Vector. Иначе ваше решение может быть отклонено.
Класс Vector не должен ничего выводить в потоки stdout и stderr. В противном случае решение может быть отклонено.
--Как будет тестироваться ваш код
Тренажёр проверит методы класса Vector, включая обеспечение гарантий безопасности исключений.
Гарантируется, что при тестировании метода Erase ему не будут передаваться невалидные итераторы и end-итератор,
приводящие к неопределённому поведению.
Код будет собран с UB и Address-санитайзерами, чтобы обнаружить ошибки работы с памятью и неопределённое поведение.
Методы класса Vector не должны ничего выводить в stdout и stderr, иначе решение будет отклонено.
Функция main будет заменена на версию из тренажёра.
--Подсказка
    Реализуйте метод Emplace — и сможете на его основе реализовать оба метода Insert.
    В методе Emplace используйте perfect forwarding параметров в конструктор объекта.
*/


int main() {
    try {
        Test1();
        Test2();
        Test3();
        Test4();
        Test5();
        Test6();
        Benchmark();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
