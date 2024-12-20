
#define WNT_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <process.h>	// _beginthreadex() e _endthreadex()
#include <conio.h>		// _getch
#include <iostream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <ctime>
#include "buffer_circular.h"

/*TO MUDANDO ESSE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;


std::atomic<bool> execucao{ true };
std::atomic<bool> bloqueioMES{ false };
std::atomic<bool> bloqueioCLP{ false };
std::atomic<bool> bloqueioRetirada{ false };
std::atomic<bool> bloqueioSetup{ false };
std::atomic<bool> bloqueioDados{ false };

HANDLE mesReadEvent;
HANDLE clpReadEvent;
HANDLE messageRetrievalEvent;
HANDLE setupDisplayEvent;
HANDLE statusDisplayEvent;
HANDLE escEvent;

HANDLE retiradaMensagemStatus;
HANDLE hMapFileLista1;
HANDLE hMapFileLista2;

BufferCircular<std::string, MAX_MESSAGES> buffer1;
BufferCircular<std::string, 100> buffer2;

BufferCircular<std::string, MAX_MESSAGES>* lista_1 = &buffer1;
BufferCircular<std::string, 100>* lista_2 = &buffer2;


const wchar_t* mutexName = L"Global\\MeuMutexCompartilhado";
const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";

HANDLE hMutex;
HANDLE hSemaphore;
HANDLE hMutexLista1;
HANDLE hMutexLista2;
BufferCircular<std::string, 1000> buffer;

HANDLE hThreadMES;
DWORD WINAPI MESFunc(LPVOID);


const wchar_t* SHARED_MEMORY_LISTA1 = L"Global\\SharedBufferLista1";
const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
const wchar_t* MUTEX_LISTA1_NAME = L"Global\\MutexLista1";
const wchar_t* MUTEX_LISTA2_NAME = L"Global\\MutexLista2";

/*------------------ FUNÇÃO GERA DADO DO MES ------------------*/
//int Nseq_dados_mes;
//
//std::string gerarDadoMES(LPVOID id) {
//
//    std::ostringstream msg;
//    if (Nseq_dados_mes > 999999) {
//        Nseq_dados_mes = 1;
//    }
//
//    Nseq_dados_mes += 1;
//    int linha = rand() % 2 + 1; // Número da linha (1 ou 2)
//    double sp_vel = (rand() % 5000) / 100.0;   // Velocidade (cm/s)
//    double sp_ench = (rand() % 10000) / 10.0;  // Enchimento (m3/min)
//    double sp_sep = (rand() % 1000) / 10.0;    // Separação (cm/s)
//
//    // Obter timestamp
//    SYSTEMTIME local;
//    GetLocalTime(&local);
//    char timestamp[9];
//    sprintf_s(timestamp, "%02d:%02d:%02d", local.wHour, local.wMinute, local.wSecond);
//
//    // Construir mensagem
//    msg << std::setw(5) << std::setfill('0') << Nseq_dados_mes << "|"  // ID formatado
//        << linha << "|"
//        << std::fixed << std::setprecision(2) << sp_vel << "|"
//        << sp_ench << "|"
//        << sp_sep << "|"
//        << timestamp;
//
//    // Exibir mensagem no console (usar c_str para conversão segura)
//    std::cout << "Dado FINAL: " << msg.str() << std::endl;
//
//    return msg.str();
//}


/*------------------ FUNÇÃO GERA DADO DO CLP ------------------*/
//int Nseq_dados_clp;
//
//std::string gerarDadoCLP(LPVOID id) {
//    std::ostringstream msg;
//
//    if (Nseq_dados_clp > 99999) {
//        Nseq_dados_clp = 1;
//    }
//
//    Nseq_dados_clp += 1;
//    int linha = rand() % 2 + 1;  // Número da linha (1 ou 2)
//    double prod_acum = (rand() % 1000000) / 10.0;  // Produção acumulada (litros)
//    double nivel_xar = (rand() % 1000) / 10.0;  // Nível do tanque de xarope (cm)
//    double nivel_agua = (rand() % 1000) / 10.0;  // Nível do tanque de água (cm)
//    int downtime = rand() % 10000;  // Downtime (minutos)
//
//    // Obter timestamp
//    SYSTEMTIME local;
//    GetLocalTime(&local);
//    char timestamp[9];
//    sprintf_s(timestamp, "%02d:%02d:%02d", local.wHour, local.wMinute, local.wSecond);
//
//    //Mensagem
//    msg << std::setw(5) << std::setfill('0') << Nseq_dados_clp << "|"
//        << linha << "|"  // Linha (LINHA)
//        << std::setw(8) << std::fixed << std::setprecision(1) << prod_acum << "|"
//        << std::setw(5) << std::fixed << std::setprecision(1) << nivel_xar << "|"
//        << std::setw(5) << std::fixed << std::setprecision(1) << nivel_agua << "|"
//        << std::setw(4) << std::setfill('0') << downtime << "|"
//        << timestamp;
//
//    // Exibir mensagem no console
//    std::cout << "Dado FINAL: " << msg.str() << std::endl;
//
//    return msg.str();
//}


void criarEventos() {
    mesReadEvent = CreateEvent(NULL, TRUE, FALSE, L"MesReadEvent");
    clpReadEvent = CreateEvent(NULL, TRUE, FALSE, L"ClpReadEvent");
    messageRetrievalEvent = CreateEvent(NULL, TRUE, FALSE, L"MessageRetrievalEvent");
    setupDisplayEvent = CreateEvent(NULL, TRUE, FALSE, L"SetupDisplayEvent");
    statusDisplayEvent = CreateEvent(NULL, TRUE, FALSE, L"StatusDisplayEvent");
    escEvent = CreateEvent(NULL, TRUE, FALSE, L"EscEvent");

    if (!mesReadEvent || !clpReadEvent || !messageRetrievalEvent ||
        !setupDisplayEvent || !statusDisplayEvent || !escEvent) {
        std::cerr << "Erro ao criar eventos. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarEventos() {
    CloseHandle(mesReadEvent);
    CloseHandle(clpReadEvent);
    CloseHandle(messageRetrievalEvent);
    CloseHandle(setupDisplayEvent);
    CloseHandle(statusDisplayEvent);
    CloseHandle(escEvent);
}

void iniciarProcesso(const std::wstring& nomeExec) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi = {};

    if (!CreateProcess(
        nomeExec.c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
        NULL,
        L"..\\Tp_parte_inicial",
        &si,
        &pi)) {
        std::wcerr << L"Erro ao iniciar " << nomeExec << L". Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void gerenciarTarefas() {
    while (execucao) {
        if (WaitForSingleObject(escEvent, 0) == WAIT_OBJECT_0) {
            execucao = false;
            std::cout << "Encerrando o sistema..." << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
/*--------------------------------=======================================-----------------------------*/
HANDLE semEscritoras;
HANDLE semLeitoras;

HANDLE hMutexLista1_clp;
BufferCircular<Mensagem, MAX_MESSAGES>* lista_mensagem_1;

std::atomic<bool> continuarProcesso{ true };

static int nseq = 0;

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

void criarSemaforos() {
    semEscritoras = CreateSemaphore(NULL, 0, 5, NULL);
    semLeitoras = CreateSemaphore(NULL, 0, 1, NULL);

    if (!semEscritoras || !semLeitoras) {
        std::cerr << "Erro na criação dos semáforos" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void liberarSemaforos() {
    CloseHandle(semEscritoras);
    CloseHandle(semLeitoras);
}

std::string criarMensagemStatus(MensagemDeStatus& mensagem) {
    std::stringstream ss;

    ss << std::setw(5) << std::setfill('0') << mensagem.nseq << " | "
        << mensagem.linha << " | "
        << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << mensagem.prod_acum << " | "
        << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << mensagem.nivel_xar << " | "
        << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << mensagem.nivel_agua << " | "
        << std::setw(5) << std::setfill('0') << mensagem.downtime << " | "
        << (mensagem.timestamp.wHour < 10 ? "0" : "") << mensagem.timestamp.wHour << ":"
        << (mensagem.timestamp.wMinute < 10 ? "0" : "") << mensagem.timestamp.wMinute << ":"
        << (mensagem.timestamp.wSecond < 10 ? "0" : "") << mensagem.timestamp.wSecond;

    return ss.str();
}

void adicionarMensagemAoBuffer() {

    MensagemDeStatus mensagem;

    WaitForSingleObject(hMutex, INFINITE);
    mensagem.nseq = nseq++;
    if (nseq > 99999) nseq = 0;

    mensagem.linha = 1 + (rand() % 2);

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << valor;
        mensagem.prod_acum = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.nivel_xar = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.nivel_agua = std::stof(ss.str());
    }

    mensagem.downtime = rand() % 10000;

    GetLocalTime(&mensagem.timestamp);

    if (!buffer1.estaCheia()) {
        buffer1.adicionarMensagem(criarMensagemStatus(mensagem));
        std::cout << "Mensagem adicionada ao buffer: NSEQ=" << mensagem.downtime << std::endl;
    }
    ReleaseMutex(hMutex);

    ReleaseSemaphore(semEscritoras, 1, NULL);
}

void monitorarEventosEscritaCLP() {
    bool bloqueado = false;

    while (continuarProcesso) {
        HANDLE eventos[] = { clpReadEvent, escEvent };
        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            bloqueado = !bloqueado;
            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
            ResetEvent(clpReadEvent);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        if (!bloqueado) {
            adicionarMensagemAoBuffer();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void monitorarEventosLeituraCLP() {
    while (continuarProcesso) {
        HANDLE eventos[] = { escEvent };
        DWORD waitResult = WaitForMultipleObjects(1, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        std::string mensagem;
        WaitForSingleObject(hMutex, INFINITE);
        if (buffer1.recuperarMensagem(mensagem) && mensagem.length() == 55) {
            std::cout << "\033[34m" << mensagem << "\033[0m" << std::endl;
        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

std::string criarMensagemSetup(MensagemDeSetup& mensagem) {
    std::stringstream ss;

    ss << std::setw(5) << std::setfill('0') << mensagem.nseq << " | "
        << mensagem.linha << " | "
        << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << mensagem.sp_vel << " | "
        << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << mensagem.sp_ench << " | "
        << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << mensagem.sp_sep << " | "
        << (mensagem.timestamp.wHour < 10 ? "0" : "") << mensagem.timestamp.wHour << ":"
        << (mensagem.timestamp.wMinute < 10 ? "0" : "") << mensagem.timestamp.wMinute << ":"
        << (mensagem.timestamp.wSecond < 10 ? "0" : "") << mensagem.timestamp.wSecond;

    return ss.str();
}

void adicionarMensagemAoBufferMES() {

    MensagemDeSetup mensagem;

    WaitForSingleObject(hMutex, INFINITE);
    mensagem.nseq = nseq++;
    if (nseq > 99999) nseq = 0;

    mensagem.linha = 1 + (rand() % 2);

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << valor;
        mensagem.sp_vel = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.sp_ench = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.sp_sep = std::stof(ss.str());
    }

    GetLocalTime(&mensagem.timestamp);

    if (!buffer1.estaCheia()) {
        buffer1.adicionarMensagem(criarMensagemSetup(mensagem));
        std::cout << "Mensagem adicionada ao buffer: NSEQ=" << nseq << std::endl;
    }
    ReleaseMutex(hMutex);

    ReleaseSemaphore(semEscritoras, 1, NULL);
}

void monitorarEventosEscritaMES() {
    bool bloqueado = false;

    while (continuarProcesso) {
        HANDLE eventos[] = { mesReadEvent, escEvent };
        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            bloqueado = !bloqueado;
            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
            ResetEvent(mesReadEvent);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        if (!bloqueado) {
            adicionarMensagemAoBufferMES();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void monitorarEventosLeituraMES() {
    while (continuarProcesso) {
        HANDLE eventos[] = { escEvent };
        DWORD waitResult = WaitForMultipleObjects(1, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        std::string mensagem_mes;

        WaitForSingleObject(hMutex, INFINITE);
        if (buffer1.recuperarMensagem(mensagem_mes) && mensagem_mes.length() != 55) {

            std::cout << "\033[35m" << mensagem_mes << "\033[0m" << std::endl;

        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {

    DWORD dwThreadId;
    hMutex = CreateMutex(NULL, FALSE, mutexName);
    if (hMutex == NULL) {
        std::cerr << "Erro ao criar o mutex. Código: " << GetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }

    hSemaphore = CreateSemaphore(NULL, 0, 1, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        std::cerr << "Erro ao criar o semáforo. Código: " << GetLastError() << std::endl;
        return 1;
    }

    criarEventos();

    iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Leitura_Teclado.exe");
    iniciarProcesso(L"..\\x64\\Debug\\Tarefa_de_Retirada_de_Mensagem.exe");
    iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Exibicao_de_Setups_de_Producao.exe");
    iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Exibicao_Dados_Processo.exe");

    /*------------- CLP --------------*/
    criarMutex();
    criarSemaforos();

    std::thread threadEscritaCLP(monitorarEventosEscritaCLP);
    std::thread threadLeituraCLP(monitorarEventosLeituraCLP);
    std::thread threadEscritaMES(monitorarEventosEscritaMES);
    std::thread threadLeituraMES(monitorarEventosLeituraMES);

    threadEscritaCLP.join();
    threadLeituraCLP.join();
    threadEscritaMES.join();
    threadLeituraMES.join();

    liberarMutex();
    liberarSemaforos();

    std::cout << "Sistema inicializado. Gerenciando tarefas..." << std::endl;
    gerenciarTarefas();

    liberarEventos();
    CloseHandle(hMutex);
    CloseHandle(hSemaphore);

    return 0;
}