#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <map>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <windows.h>
#include <io.h> 
#include <fcntl.h> 

std::atomic<bool> execucao{ true };
std::atomic<bool> bloqueioMES{ false };
std::atomic<bool> bloqueioCLP{ false };
std::atomic<bool> bloqueioRetirada{ false };
std::atomic<bool> bloqueioSetup{ false };
std::atomic<bool> bloqueioDados{ false };

HANDLE mesReadEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"MesReadEvent");
HANDLE clpReadEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"ClpReadEvent");
HANDLE messageRetrievalEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"MessageRetrievalEvent");
HANDLE setupDisplayEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"SetupDisplayEvent");
HANDLE statusDisplayEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, L"StatusDisplayEvent");
HANDLE escEvent = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, L"EscEvent");

void exibirEstado(const std::wstring& tarefa, bool bloqueado) {
    std::wcout << tarefa << L" está " << (bloqueado ? L"BLOQUEADA" : L"EM EXECUÇÃO") << std::endl;
}

void tarefaLeituraTeclado() {
    std::map<char, std::pair<std::wstring, std::atomic<bool>*>> teclas = {
        {'M', {L"Tarefa de leitura MES", &bloqueioMES}},
        {'c', {L"Tarefa de leitura CLP", &bloqueioCLP}},
        {'r', {L"Tarefa de retirada de mensagens", &bloqueioRetirada}},
        {'s', {L"Tarefa de setup de produção", &bloqueioSetup}},
        {'p', {L"Tarefa de exibição de dados de processo", &bloqueioDados}},
    };

    std::wcout << L"Pressione as teclas definidas para controle das tarefas ou ESC para sair.\n";
    char tecla;
    while (execucao) {
        if (_kbhit()) {
            tecla = _getch();
            if (tecla == 27) {
                execucao = false;
                SetEvent(escEvent);
                std::wcout << L"Encerrando todas as tarefas...\n";
                break;
            }
            else if (teclas.find(tecla) != teclas.end()) {
                auto& tarefa = teclas[tecla];

                *(tarefa.second) = !*(tarefa.second);
                exibirEstado(tarefa.first, *(tarefa.second));

                if (tecla == 'M') SetEvent(mesReadEvent);
                else if (tecla == 'c') SetEvent(clpReadEvent);
                else if (tecla == 'r') SetEvent(messageRetrievalEvent);
                else if (tecla == 's') SetEvent(setupDisplayEvent);
                else if (tecla == 'p') SetEvent(statusDisplayEvent);
            }
            else {
                std::wcout << L"Tecla inválida.\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    tarefaLeituraTeclado();
    return 0;
}
