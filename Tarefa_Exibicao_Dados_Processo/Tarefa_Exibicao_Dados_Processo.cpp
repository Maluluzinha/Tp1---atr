#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <windows.h>
#include "../Tp_parte_inicial/buffer_circular.h"

const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";
const wchar_t* MUTEX_NAME = L"Global\\MeuMutexCompartilhado";
const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
const wchar_t* MUTEX_LISTA2_NAME = L"Global\\MutexLista2"; // Corrigido para o nome do mutex da lista 2

BufferCircular<MensagemDeStatus, 100>* lista_22;

HANDLE hSemaphore;
HANDLE hMutex;

void criarMutex() {
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_LISTA2_NAME); // Usando o nome correto para o mutex
    if (hMutex == NULL) {
        std::cerr << "Erro ao abrir o mutex. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarMutex() {
    CloseHandle(hMutex);
}

void tarefaExibicaoDeDadosDeProcesso() {
    while (true) {
        std::cout << "Aguardando semáforo..." << std::endl;
        WaitForSingleObject(hSemaphore, INFINITE); // Espera o semáforo ser sinalizado
        std::cout << "Semáforo sinalizado, processando mensagem..." << std::endl;

        MensagemDeStatus mensagem;

        WaitForSingleObject(hMutex, INFINITE); // Aguarda o mutex para acessar o buffer

        if (lista_22->recuperarMensagem(mensagem)) {
            std::cout << std::setfill('0') << std::setw(2) << mensagem.timestamp.wHour << ":"
                << std::setfill('0') << std::setw(2) << mensagem.timestamp.wMinute << ":"
                << std::setfill('0') << std::setw(2) << mensagem.timestamp.wSecond << " "
                << "NSEQ: " << mensagem.nseq
                << " LINHA: " << mensagem.linha
                << " PROD: " << std::fixed << std::setprecision(1) << mensagem.prod_acum
                << " N_XAR: " << mensagem.nivel_xar
                << " N_H2O: " << mensagem.nivel_agua
                << " DOWNTIME: " << mensagem.downtime
                << std::endl;
        }
        else {
            std::cerr << "Erro: Sem mensagens disponíveis no buffer lista_2." << std::endl;
        }

        ReleaseMutex(hMutex); // Libera o mutex após o processamento

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Espera 100 ms
    }
}

int main() {
    std::cout << "Iniciando a tarefa de exibição de dados de processo..." << std::endl;

    // Abrir o semáforo
    hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        std::cerr << "Erro ao abrir o semáforo. Código: " << GetLastError() << std::endl;
        return 1;
    }

    // Abrir o mapeamento de memória compartilhada
    HANDLE hMapFileLista2 = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_LISTA2);
    if (hMapFileLista2 == NULL) {
        std::cerr << "Erro ao abrir a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Mapear a memória compartilhada
    lista_22 = (BufferCircular<MensagemDeStatus, 100>*)MapViewOfFile(
        hMapFileLista2, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BufferCircular<MensagemDeStatus, 100>)
    );
    if (lista_22 == NULL) {
        std::cerr << "Erro ao mapear a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Criar o mutex
    criarMutex();

    // Iniciar a tarefa de exibição de dados
    tarefaExibicaoDeDadosDeProcesso();

    // Liberar os recursos
    liberarMutex();
    CloseHandle(hSemaphore);
    UnmapViewOfFile(lista_22);
    CloseHandle(hMapFileLista2);

    return 0;
}
