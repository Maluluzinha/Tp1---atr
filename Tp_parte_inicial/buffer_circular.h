#ifndef BUFFER_CIRCULAR_H
#define BUFFER_CIRCULAR_H

#include <Windows.h>
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <condition_variable>

constexpr int MAX_MESSAGES = 1000;
extern HANDLE hMutexLista1;  // Mutex para lista_1
extern HANDLE hMutexLista2;  // Mutex para lista_2

struct MensagemDeSetup {
    int nseq;
    int line;
    float sp_vel;
    float sp_ench;
    float sp_sep;
    SYSTEMTIME timestamp;
};

struct MensagemDeStatus {
    int nseq;
    int linha;
    float prod_acum;
    float nivel_xar;
    float nivel_agua;
    int downtime;
    SYSTEMTIME timestamp;
};

union Mensagem {
    MensagemDeSetup setup;
    MensagemDeStatus status;
};

template <typename T, size_t Size>
class BufferCircular {
private:
    std::array<T, Size> buffer;
    size_t head;
    size_t tail;
    std::mutex mtx;
    std::condition_variable cv;

public:
    BufferCircular() : head(0), tail(0) {}

    void adicionarMensagem(const T& mensagem) {
        std::lock_guard<std::mutex> lock(mtx);
        buffer[tail] = mensagem;
        tail = (tail + 1) % Size;
        if (tail == head) {
            head = (head + 1) % Size;
        }
        cv.notify_one();
    }

    bool recuperarMensagem(T& mensagem) {
        std::lock_guard<std::mutex> lock(mtx);
        if (head == tail) {
            return false;
        }
        mensagem = buffer[head];
        head = (head + 1) % Size;
        return true;
    }

    void aguardarMensagem(T& mensagem) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return head != tail; });
        mensagem = buffer[head];
        head = (head + 1) % Size;
    }

    bool estaCheia() const {
        return (tail + 1) % Size == head;
    }
};

extern BufferCircular<Mensagem, MAX_MESSAGES>* lista_1;
extern BufferCircular<MensagemDeStatus, 100>* lista_2;

#endif
