#ifndef SINGLEEMIT_HPP
#define SINGLEEMIT_HPP

#include <cassert>
#include <exception>
#include <stdexcept>

template<class T>
class SingletonEmit {

public:
    static T* instance() {
        if (!m_instance) {
            m_instance = new T;
        }

        if (m_instance == nullptr) {
            throw std::runtime_error("'m_instance' is a nullptr! Aborting!");
        }

        return m_instance;
    }

protected:
    SingletonEmit();
    ~SingletonEmit();

private:
    SingletonEmit(SingletonEmit const&);
    SingletonEmit& operator=(SingletonEmit const&);
    static T* m_instance;
};

template <class T> T* SingletonEmit<T>::m_instance = NULL;

#endif // SINGLEEMIT_HPP
