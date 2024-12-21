
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
    
    setupDisplayEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"SetupDisplayEvent");
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
            SetConsoleTextAttribute(cout_handle, FOREGROUND_BLUE);
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