//#include <iostream>
//#include <iomanip>
//#include <thread>
//#include <chrono>
//#include <windows.h>
//#include "../Tp_parte_inicial/buffer_circular.h"
//
//const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";
//const wchar_t* MUTEX_NAME = L"Global\\MeuMutexCompartilhado";
//const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
//const wchar_t* MUTEX_LISTA2_NAME = L"Global\\MutexLista2"; // Corrigido para o nome do mutex da lista 2
//
//BufferCircular<MensagemDeStatus, 100>* lista_22;
//
//HANDLE hSemaphore;
//HANDLE hMutex;
//
//void criarMutex() {
//    hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_LISTA2_NAME); // Usando o nome correto para o mutex
//    if (hMutex == NULL) {
//        std::cerr << "Erro ao abrir o mutex. Código: " << GetLastError() << std::endl;
//        exit(EXIT_FAILURE);
//    }
//}
//
//void liberarMutex() {
//    CloseHandle(hMutex);
//}
//
//void tarefaExibicaoDeDadosDeProcesso() {
//    while (true) {
//        std::cout << "Aguardando semáforo..." << std::endl;
//        WaitForSingleObject(hSemaphore, INFINITE); // Espera o semáforo ser sinalizado
//        std::cout << "Semáforo sinalizado, processando mensagem..." << std::endl;
//
//        MensagemDeStatus mensagem;
//
//        WaitForSingleObject(hMutex, INFINITE); // Aguarda o mutex para acessar o buffer
//
//        if (lista_22->recuperarMensagem(mensagem)) {
//            std::cout << std::setfill('0') << std::setw(2) << mensagem.timestamp.wHour << ":"
//                << std::setfill('0') << std::setw(2) << mensagem.timestamp.wMinute << ":"
//                << std::setfill('0') << std::setw(2) << mensagem.timestamp.wSecond << " "
//                << "NSEQ: " << mensagem.nseq
//                << " LINHA: " << mensagem.linha
//                << " PROD: " << std::fixed << std::setprecision(1) << mensagem.prod_acum
//                << " N_XAR: " << mensagem.nivel_xar
//                << " N_H2O: " << mensagem.nivel_agua
//                << " DOWNTIME: " << mensagem.downtime
//                << std::endl;
//        }
//        else {
//            std::cerr << "Erro: Sem mensagens disponíveis no buffer lista_2." << std::endl;
//        }
//
//        ReleaseMutex(hMutex); // Libera o mutex após o processamento
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Espera 100 ms
//    }
//}
//
//int main() {
//    std::cout << "Iniciando a tarefa de exibição de dados de processo..." << std::endl;
//
//    // Abrir o semáforo
//    hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME);
//    if (hSemaphore == NULL) {
//        std::cerr << "Erro ao abrir o semáforo. Código: " << GetLastError() << std::endl;
//        return 1;
//    }
//
//    //// Abrir o mapeamento de memória compartilhada
//    //HANDLE hMapFileLista2 = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_LISTA2);
//    //if (hMapFileLista2 == NULL) {
//    //    std::cerr << "Erro ao abrir a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
//    //    exit(EXIT_FAILURE);
//    //}
//
//    //// Mapear a memória compartilhada
//    //lista_22 = (BufferCircular<MensagemDeStatus, 100>*)MapViewOfFile(
//    //    hMapFileLista2, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BufferCircular<MensagemDeStatus, 100>)
//    //);
//    //if (lista_22 == NULL) {
//    //    std::cerr << "Erro ao mapear a memória compartilhada para lista_2. Código: " << GetLastError() << std::endl;
//    //    exit(EXIT_FAILURE);
//    //}
//
//    // Criar o mutex
//    criarMutex();
//
//    // Iniciar a tarefa de exibição de dados
//    tarefaExibicaoDeDadosDeProcesso();
//
//    // Liberar os recursos
//    liberarMutex();
//    CloseHandle(hSemaphore);
//    //UnmapViewOfFile(lista_22);
//    //CloseHandle(hMapFileLista2);
//
//    return 0;
//}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#include "../Tp_parte_inicial/buffer_circular.h"

//Define cores de texto
#define WHITE  FOREGROUND_RED   | FOREGROUND_GREEN      | FOREGROUND_BLUE
#define BLUE   FOREGROUND_BLUE| FOREGROUND_INTENSITY
#define RED    FOREGROUND_RED   | FOREGROUND_INTENSITY

HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

HANDLE hMutex;
HANDLE statusDisplayEvent;
HANDLE escEvent;
std::atomic<bool> continuarProcesso{ true };

void criarEventos() {
   
    statusDisplayEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"StatusDisplayEvent");
    escEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"EscEvent");

    if (!statusDisplayEvent || !escEvent) {
        std::cerr << "Erro ao criar eventos. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarEventos() {
    CloseHandle(statusDisplayEvent);
    CloseHandle(escEvent);
}

void monitorarEvento() {
    bool bloqueado = false;

    while (continuarProcesso) {
        HANDLE eventos[] = { statusDisplayEvent, escEvent };
        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            bloqueado = !bloqueado;
            SetConsoleTextAttribute(cout_handle, FOREGROUND_BLUE);
            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
            ResetEvent(statusDisplayEvent);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        if (!bloqueado) {
            SetConsoleTextAttribute(cout_handle, FOREGROUND_GREEN);
            std::cout << ("Processo de exibicao de dados do processo executando...") << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

int main() {
    criarEventos();

    std::thread threadEscrita(monitorarEvento);

    threadEscrita.join();

    liberarEventos();
    return 0;

}