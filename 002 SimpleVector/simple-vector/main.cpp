#include <cassert>
#include <iostream>
#include <iomanip>
#include <numeric>

#include "simple_vector.h"
// Tests
#include "tests.h"

using namespace std;

/*
Задание
Улучшите свой вектор, добавив возможность перемещения.
Используйте предлагаемые тесты и допишите свои, чтобы удостовериться,
что все нужные методы поддерживают не только копирование, но и перемещение.
Используйте знания, полученные в последних уроках, и разберитесь,
какие методы добавить в уже существующие классы, чтобы поддержать возможность перемещения.
--Подсказка
Следуйте за ошибками компиляции. Они выведут вас на верную дорогу.
*/


/*
Задание
Reserve задает ёмкость вектора. Этот метод повышает эффективность кода в случае,
когда пользователь заранее знает хотя бы приблизительное количество элементов в векторе.
Reserve сразу выделяет нужное количество памяти.
При добавлении новых элементов в вектор копирование будет происходить или значительно реже или совсем не будет.
Если new_capacity больше текущей capacity, память должна быть перевыделена,
а элементы вектора скопированы в новый отрезок памяти.
Кроме этого реализуйте возможность вызвать конструктор SimpleVector,
который будет сразу резервировать нужное количество памяти.
--Подсказка
Не забудьте обновить указатель на начало вектора и текущую ёмкость.
Для реализации конструктора с резервированием вам понадобится дополнительный класс-обёртка,
чтобы компилятор мог разобраться и вызвать правильный конструктор.
Этот конструктор должен принимать по значению объект этого класса-обёртки.
Тогда функция (не метод!) будет иметь следующую сигнатуру:
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
};
*/

inline void MyTestAsserts() {
    std::cout << left << setw(20) << "Test assert" << "-Start!-" << std::endl << std::endl;
    // Вставка или удаление элементов вне диапазона вектора
    {
        SimpleVector<int> v{1, 2, 3, 4};

        //v.Insert(v.end() + 1, 42);
        //v.Insert(v.begin() + 4, 42);
        //v.Insert(v.begin() - 1, 42);
        //v.Insert(v.end() - 4, 42);

        //v.Erase(v.end() + 1);
        //v.Erase(v.begin() + 4);
        //v.Erase(v.begin() - 1);
        //v.Erase(v.end() - 4);

        assert(v[0] == 1);
        assert(v[3] == 4);
        v.Insert(v.begin(), 42);
        assert(v[0] == 42);
        assert(v[4] == 4);
        v.Erase(v.begin());
        assert(v[0] == 1);
        v.Erase(v.end());
        assert(v[2] == 3);

        v.Clear();
        std::cout << "Assert's #1" << " ...skipped!" << std::endl;
    }

    // Удаление элементов из пустого вектора
    {
        SimpleVector<int> v{};
        //v.Erase(v.begin());
        //v.PopBack();
        v.Clear();
        std::cout << "Assert's #2" << " ...skipped!" << std::endl;
    }

    // Доступ к элементу по неправильному индексу
    {
        SimpleVector<int> v{1, 2, 3, 4};
        v.operator[](4);
        //v.operator[](5);
        v.At(3);
        //v.At(4);
        v.Clear();
        std::cout << "Assert's #3" << " ...skipped!" << std::endl;
    }

    /*
    // Присваивание
    {
        SimpleVector<int> v1{10, 42};
        SimpleVector<int> v2{10, 42};
        SimpleVector<int> v3;
        v3 = v1;
        assert(v3 == v1);
        v1 = v1;
    }
    */
    std::cout << std::endl << left << setw(20) << "All assert's tests" << " -Skipped!-" << std::endl;
}

int main() {

    SetTest1();
    SetTest2();
    cout << "< OLD TESTS 1 > -OK-" << endl << endl;
    TestReserveConstructor();
    TestReserveMethod();
    cout << "< OLD TESTS 2 > -OK-" << endl << endl;

    TestTemporaryObjConstructor();
    TestTemporaryObjOperator();
    TestNamedMoveConstructor();
    TestNamedMoveOperator();
    TestNoncopiableMoveConstructor();
    TestNoncopiablePushBack();
    TestNoncopiableInsert();
    TestNoncopiableErase();
    cout << "< NEW TESTS > -OK-" << endl << endl;

    MyTestAsserts();
    cout << "< MY TESTS > -OK-" << endl << endl;
    return 0;
}
