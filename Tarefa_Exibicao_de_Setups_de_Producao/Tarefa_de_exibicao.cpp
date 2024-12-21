//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <iostream>
//#include <string>
//#include <mutex>
//#include <thread>
//#include <chrono>
//
//const wchar_t* pipeName = L"\\\\.\\pipe\\MeuPipe";
//
//void conectarPipeEExibirDados() {
//    HANDLE hPipe = CreateFile(
//        pipeName,               // Nome da pipe
//        GENERIC_READ,           // Acesso de leitura
//        0,                      // Sem compartilhamento
//        NULL,                   // Segurança padrão
//        OPEN_EXISTING,          // Abre a pipe existente
//        0,                      // Atributos padrão
//        NULL);                  // Sem modelo de atributo
//
//    if (hPipe == INVALID_HANDLE_VALUE) {
//        std::cerr << "Erro ao conectar à pipe. Código: " << GetLastError() << std::endl;
//        return;
//    }
//
//    std::cout << "Conexão à pipe estabelecida." << std::endl;
//
//    while (true) {
//        char buffer[512];
//        DWORD bytesRead;
//
//        // Tenta ler dados da pipe
//        if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
//            buffer[bytesRead] = '\0'; // Garante que o buffer seja terminado com '\0'
//
//            std::cout << "Mensagem recebida da pipe: " << buffer << std::endl;
//
//            // Formata e exibe os dados
//            std::cout << "HH:MM:SS " << buffer << std::endl;
//        }
//        else {
//            DWORD error = GetLastError();
//            if (error == ERROR_BROKEN_PIPE) {
//                std::cout << "Conexão com a pipe foi encerrada pelo servidor." << std::endl;
//                break;
//            }
//            else {
//                std::cerr << "Erro ao ler da pipe. Código: " << error << std::endl;
//            }
//        }
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Aguarda antes de tentar novamente
//    }
//
//    CloseHandle(hPipe);
//}
//
//int main() {
//    std::cout << "Cliente de leitura iniciado..." << std::endl;
//
//    conectarPipeEExibirDados();
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
HANDLE setupDisplayEvent;
HANDLE escEvent;
std::atomic<bool> continuarProcesso{ true };

void criarEventos() {
    /*setupDisplayEvent = OpenEvent(SYNCHRONIZE, FALSE, L"setupDisplayEvent");
    escEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EscEvent");*/
    //setupDisplayEvent = CreateEvent(NULL, TRUE, FALSE, L"setupDisplayEvent");

    setupDisplayEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"SetupDisplayEvent");
    escEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"EscEvent");

    if (!setupDisplayEvent || !escEvent) {
        std::cerr << "Erro ao criar eventos. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarEventos() {
    CloseHandle(setupDisplayEvent);
    CloseHandle(escEvent);
}

void monitorarEvento() {
    bool bloqueado = false;

    while (continuarProcesso) {
        HANDLE eventos[] = { setupDisplayEvent, escEvent };
        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            bloqueado = !bloqueado;
            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
            ResetEvent(setupDisplayEvent);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        if (!bloqueado) {
            SetConsoleTextAttribute(cout_handle, FOREGROUND_GREEN);
            std::cout << ("Processo de exibicao do setup executando...") << std::endl;
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