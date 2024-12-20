#include <windows.h>
#include <iostream>
#include <string>
#include <mutex>
#include "../Tp_parte_inicial/buffer_circular.h"

const wchar_t* pipeName = L"\\\\.\\pipe\\MeuPipe";
const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";

HANDLE hSemaphore;
HANDLE hMutex;

const wchar_t* SHARED_MEMORY_LISTA1 = L"Global\\SharedBufferLista1";
const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
const wchar_t* MUTEX_LISTA1_NAME = L"Global\\MutexLista1";
const wchar_t* MUTEX_LISTA2_NAME = L"Global\\MutexLista2";

BufferCircular<Mensagem, MAX_MESSAGES>* lista_mensagem_retirada;
BufferCircular<MensagemDeStatus, 100>* lista_mensagem_status_retirada;

void criarMutex() {
    hMutex = CreateMutex(NULL, FALSE, L"Global\\MeuMutexCompartilhado");
    if (hMutex == NULL) {
        std::cerr << "Erro ao criar o mutex. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarMutex() {
    CloseHandle(hMutex);
}

void criarPipeEEnviarDados() {
    HANDLE hPipe = CreateNamedPipe(
        pipeName,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE |
        PIPE_WAIT,
        1,
        0,
        0,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Erro ao criar o pipe. Código: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Pipe criado. Aguardando conexão..." << std::endl;

    if (!ConnectNamedPipe(hPipe, NULL)) {
        std::cerr << "Erro ao conectar ao pipe. Código: " << GetLastError() << std::endl;
        CloseHandle(hPipe);
        return;
    }

    std::cout << "Cliente conectado ao pipe." << std::endl;

    while (true) {
        Mensagem mensagem;
        WaitForSingleObject(hMutex, INFINITE);

        if (lista_mensagem_retirada->recuperarMensagem(mensagem)) {
            std::string mensagemFormatada_setup;
            std::string mensagemFormatada_status;

            // Verifica se há dados válidos no status
            if (mensagem.status.prod_acum != 0.0f) {
                mensagemFormatada_status =
                    "nseq: " + std::to_string(mensagem.status.nseq) +
                    ", linha: " + std::to_string(mensagem.status.linha) +
                    ", prod_acum: " + std::to_string(mensagem.status.prod_acum) +
                    ", nivel_xar: " + std::to_string(mensagem.status.nivel_xar) +
                    ", nivel_agua: " + std::to_string(mensagem.status.nivel_agua) +
                    ", downtime: " + std::to_string(mensagem.status.downtime) +
                    ", timestamp: " + std::to_string(mensagem.status.timestamp.wHour) + ":" +
                    std::to_string(mensagem.status.timestamp.wMinute) + ":" +
                    std::to_string(mensagem.status.timestamp.wSecond);

                // Adiciona a mensagem de status diretamente na lista_2
                lista_mensagem_status_retirada->adicionarMensagem(mensagem.status);

                // Sinaliza o semáforo
                if (!ReleaseSemaphore(hSemaphore, 1, NULL)) {
                    std::cerr << "Erro ao sinalizar o semáforo. Código: " << GetLastError() << std::endl;
                }
            }
            else if (mensagem.setup.sp_vel != 0.0f) {
                mensagemFormatada_setup =
                    "nseq: " + std::to_string(mensagem.setup.nseq) +
                    ", linha: " + std::to_string(mensagem.setup.linha) +
                    ", sp_vel: " + std::to_string(mensagem.setup.sp_vel) +
                    ", sp_ench: " + std::to_string(mensagem.setup.sp_ench) +
                    ", sp_sep: " + std::to_string(mensagem.setup.sp_sep) +
                    ", timestamp: " + std::to_string(mensagem.setup.timestamp.wHour) + ":" +
                    std::to_string(mensagem.setup.timestamp.wMinute) + ":" +
                    std::to_string(mensagem.setup.timestamp.wSecond);
            }

            // Envia a string formatada para o pipe
            DWORD bytesWritten;
            if (!WriteFile(hPipe, mensagemFormatada_status.c_str(), mensagemFormatada_status.size() + 1, &bytesWritten, NULL)) {
                std::cerr << "Erro ao escrever no pipe. Código: " << GetLastError() << std::endl;
                break;
            }

            std::cout << mensagemFormatada_status << std::endl;
            std::cout << "Mensagem enviada pelo pipe: " << mensagemFormatada_status << std::endl;
        }
        else {
            std::cout << "Buffer circular vazio, aguardando dados..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        ReleaseMutex(hMutex);
    }

    CloseHandle(hPipe);
}

int main() {
    std::cout << "Iniciando o programa..." << std::endl;

    hSemaphore = OpenSemaphore(
        SEMAPHORE_ALL_ACCESS,
        FALSE,
        SEMAPHORE_NAME
    );

    if (hSemaphore == NULL) {
        std::cerr << "Erro ao abrir o semáforo. Código: " << GetLastError() << std::endl;
        return 1;
    }

    HANDLE hMapFileLista1 = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_LISTA1
    );
    if (hMapFileLista1 == NULL) {
        std::cerr << "Erro ao abrir a memória compartilhada para lista_1. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    lista_mensagem_retirada = (BufferCircular<Mensagem, MAX_MESSAGES>*)MapViewOfFile(
        hMapFileLista1,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(BufferCircular<Mensagem, MAX_MESSAGES>)
    );
    if (lista_mensagem_retirada == NULL) {
        std::cerr << "Erro ao mapear a memória compartilhada para lista_1. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Abrir a memória compartilhada para lista_2
    HANDLE hMapFileLista2 = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_LISTA2
    );
    if (hMapFileLista2 == NULL) {
        std::cerr << "Erro ao abrir a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    lista_mensagem_status_retirada = (BufferCircular<MensagemDeStatus, 100>*)MapViewOfFile(
        hMapFileLista2,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(BufferCircular<MensagemDeStatus, 100>)
    );
    if (lista_mensagem_status_retirada == NULL) {
        std::cerr << "Erro ao mapear a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }


    criarMutex();

    criarPipeEEnviarDados();

    liberarMutex();

    UnmapViewOfFile(lista_mensagem_retirada);
    UnmapViewOfFile(lista_mensagem_status_retirada);

    CloseHandle(hMapFileLista1);
    CloseHandle(hMapFileLista2);

    return 0;
}
