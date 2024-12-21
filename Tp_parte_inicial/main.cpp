
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

BufferCircular<MensagemUnificada, MAX_MESSAGES>* bufferUnificado;

const wchar_t* mutexName = L"Global\\MeuMutexCompartilhado";
const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";

HANDLE hMutex;
HANDLE hSemaphore;
HANDLE hMutexLista1;
HANDLE hMutexLista2;

HANDLE hThreadMES;
DWORD WINAPI MESFunc(LPVOID);	// declaração da função do MES

//const wchar_t* SHARED_MEMORY_LISTA1 = L"Global\\SharedBufferLista1";
//const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
const wchar_t* MUTEX_LISTA1_NAME = L"Global\\MutexLista1";
const wchar_t* MUTEX_LISTA2_NAME = L"Global\\MutexLista2";

/*------------------ FUNÇÃO GERA DADO DO MES ------------------*/
int Nseq_dados_mes;
std::string gerarDadoMES(LPVOID id) {

    std::ostringstream msg;
    if (Nseq_dados_mes > 999999) {
        Nseq_dados_mes = 1;
    }

    Nseq_dados_mes += 1;
    int linha = rand() % 2 + 1; // Número da linha (1 ou 2)
    double sp_vel = (rand() % 5000) / 100.0;   // Velocidade (cm/s)
    double sp_ench = (rand() % 10000) / 10.0;  // Enchimento (m3/min)
    double sp_sep = (rand() % 1000) / 10.0;    // Separação (cm/s)

    // Obter timestamp
    SYSTEMTIME local;
    GetLocalTime(&local);
    char timestamp[9];
    sprintf_s(timestamp, "%02d:%02d:%02d", local.wHour, local.wMinute, local.wSecond);

    // Construir mensagem
    msg << std::setw(5) << std::setfill('0') << Nseq_dados_mes << "|"  // ID formatado
        << linha << "|"
        << std::fixed << std::setprecision(2) << sp_vel << "|"
        << sp_ench << "|"
        << sp_sep << "|"
        << timestamp;

    // Exibir mensagem no console (usar c_str para conversão segura)
    std::cout << "Dado FINAL: " << msg.str() << std::endl;

    return msg.str();
}

/*------------------ FUNÇÃO GERA DADO DO CLP ------------------*/
int Nseq_dados_clp;

std::string gerarDadoCLP(LPVOID id) {
    std::ostringstream msg;

    if (Nseq_dados_clp > 99999) {
        Nseq_dados_clp = 1;
    }

    Nseq_dados_clp += 1;
    int linha = rand() % 2 + 1;  // Número da linha (1 ou 2)
    double prod_acum = (rand() % 1000000) / 10.0;  // Produção acumulada (litros)
    double nivel_xar = (rand() % 1000) / 10.0;  // Nível do tanque de xarope (cm)
    double nivel_agua = (rand() % 1000) / 10.0;  // Nível do tanque de água (cm)
    int downtime = rand() % 10000;  // Downtime (minutos)

    // Obter timestamp
    SYSTEMTIME local;
    GetLocalTime(&local);
    char timestamp[9];
    sprintf_s(timestamp, "%02d:%02d:%02d", local.wHour, local.wMinute, local.wSecond);

    //Mensagem
    msg << std::setw(5) << std::setfill('0') << Nseq_dados_clp << "|"
        << linha << "|"  // Linha (LINHA)
        << std::setw(8) << std::fixed << std::setprecision(1) << prod_acum << "|"
        << std::setw(5) << std::fixed << std::setprecision(1) << nivel_xar << "|"
        << std::setw(5) << std::fixed << std::setprecision(1) << nivel_agua << "|"
        << std::setw(4) << std::setfill('0') << downtime << "|"
        << timestamp;

    // Exibir mensagem no console
    std::cout << "Dado FINAL: " << msg.str() << std::endl;

    return msg.str();
}

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

std::atomic<bool> continuarProcesso{ true };

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

static int nseq = 0;

void adicionarMensagemAoBuffer() {

    MensagemUnificada mensagem{};

    mensagem.tipo = TipoMensagem::CLP;

    WaitForSingleObject(hMutex, INFINITE);
    mensagem.nseq = nseq++;
    if (nseq > 99999) nseq = 0;

    mensagem.linha = 1 + (rand() % 2);

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << valor;
        mensagem.valor1 = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.valor2 = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.valor3 = std::stof(ss.str());
    }

    mensagem.valor4 = rand() % 10000;

    GetLocalTime(&mensagem.timestamp);

    if (!bufferUnificado->estaCheia()) {
        bufferUnificado->adicionarMensagem(mensagem);
        //std::cout << "Mensagem adicionada ao buffer: NSEQ=" << mensagem.valor4 << std::endl;
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

        MensagemUnificada mensagem;

        WaitForSingleObject(hMutex, INFINITE);
        if (bufferUnificado->recuperarMensagem(mensagem)) {
            if (mensagem.tipo == TipoMensagem::CLP) {
                std::cout << std::setfill('0')
                    << std::setw(5) << mensagem.nseq << "|"
                    << mensagem.linha << "|"
                    << std::fixed << std::setprecision(1)
                    << std::setw(8) << std::setfill('0') << mensagem.valor1 << "|"
                    << std::setw(5) << std::setfill('0') << mensagem.valor2 << "|"
                    << std::setw(5) << std::setfill('0') << mensagem.valor3 << "|"
                    << std::setw(4) << std::setfill('0') << mensagem.valor4 << "|"
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wHour << ":"
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wMinute << ":"
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wSecond
                    << std::endl;
            }
        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void adicionarMensagemAoBufferMES() {

    MensagemUnificada mensagem{};

    mensagem.tipo = TipoMensagem::MES;

    WaitForSingleObject(hMutex, INFINITE);
    mensagem.nseq = nseq++;
    if (nseq > 99999) nseq = 0;

    mensagem.linha = 1 + (rand() % 2);

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << valor;
        mensagem.valor1 = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.valor2 = std::stof(ss.str());
    }

    {
        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
        mensagem.valor3 = std::stof(ss.str());
    }

    GetLocalTime(&mensagem.timestamp);

    
    if (!bufferUnificado->estaCheia()) {
        bufferUnificado->adicionarMensagem(mensagem);
        //std::cout << "Mensagem adicionada ao buffer: NSEQ=" << 0000 << std::endl;
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

        MensagemUnificada mensagem;

        WaitForSingleObject(hMutex, INFINITE);
        if (bufferUnificado->recuperarMensagem(mensagem)) {
            /*if (mensagem.tipo == TipoMensagem::MES) {
                std::cout << std::setfill('0')
                    << std::setw(5) << mensagem.nseq << "|"
                    << mensagem.linha << "|"
                    << std::fixed << std::setfill('0') << std::setprecision(2)
                    << mensagem.valor1 << "|" 
                    << std::fixed << std::setfill('0') << std::setprecision(2)
                    << mensagem.valor2 << "|" 
                    << std::fixed << std::setfill('0') << std::setprecision(2)
                    << mensagem.valor3 << "|"  
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wHour << ":"
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wMinute << ":"
                    << std::setw(2) << std::setfill('0') << mensagem.timestamp.wSecond
                    << std::endl;
            }*/
        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void retirarMensagemDoBuffer() {
    MensagemUnificada mensagem;
    const std::string LILAC = "\033[95m";
    const std::string RESET = "\033[0m";

    WaitForSingleObject(hMutex, INFINITE);
    if (bufferUnificado->recuperarMensagem(mensagem)) {
        if (mensagem.tipo == TipoMensagem::MES) {
            std::cout << RESET
                << std::setfill('0')
                << std::setw(5) << mensagem.nseq << "|"
                << mensagem.linha << "|"
                << std::setw(5) << std::setfill('0')
                << mensagem.valor1 << "|"
                << std::setw(6) << std::setfill('0')
                << mensagem.valor2 << "|"
                << std::setw(5) << std::setfill('0')
                << mensagem.valor3 << "|"
                << std::setfill('0')
                << std::setw(2) << mensagem.timestamp.wHour << ":"
                << std::setw(2) << mensagem.timestamp.wMinute << ":"
                << std::setw(2) << mensagem.timestamp.wSecond
                << LILAC << std::endl;
        }
        else {

            std::cout << LILAC
                << std::setfill('0')
                << std::setw(5) << mensagem.nseq << "|"
                << mensagem.linha << "|"
                << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1)
                << mensagem.valor1 << "|"
                << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1) 
                << mensagem.valor2 << "|"
                << std::setw(5) << std::setfill('0') << std::fixed << std::setprecision(1)
                << mensagem.valor3 << "|"
                << std::setw(4) << std::setfill('0') << std::fixed << std::setprecision(0)
                << mensagem.valor4 << "|"
                << std::setw(2) << std::setfill('0') << mensagem.timestamp.wHour << ":"
                << std::setw(2) << std::setfill('0') << mensagem.timestamp.wMinute << ":"
                << std::setw(2) << std::setfill('0') << mensagem.timestamp.wSecond
                << RESET << std::endl;
        }
    }
    ReleaseMutex(hMutex);
}

void monitorarEventosRetiradaDeMensagens() {
    bool bloqueado = false;

    while (continuarProcesso) {
        HANDLE eventos[] = { messageRetrievalEvent, escEvent };
        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);

        if (waitResult == WAIT_OBJECT_0) {
            bloqueado = !bloqueado;
            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
            ResetEvent(messageRetrievalEvent);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "Encerrando processo..." << std::endl;
            continuarProcesso = false;
            exit(0);
        }

        if (!bloqueado) {
            retirarMensagemDoBuffer();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void liberarRecursos() {
    if (bufferUnificado) {
        delete bufferUnificado;
        bufferUnificado = nullptr;
    }

    liberarEventos();

    if (hMutex) CloseHandle(hMutex);
    if (hMutexLista1) CloseHandle(hMutexLista1);
    if (hMutexLista2) CloseHandle(hMutexLista2);
    if (hMutexLista1_clp) CloseHandle(hMutexLista1_clp);

    if (hSemaphore) CloseHandle(hSemaphore);
    if (semEscritoras) CloseHandle(semEscritoras);
    if (semLeitoras) CloseHandle(semLeitoras);

    if (hMapFileLista1) CloseHandle(hMapFileLista1);
    if (hMapFileLista2) CloseHandle(hMapFileLista2);

    if (retiradaMensagemStatus) CloseHandle(retiradaMensagemStatus);
}

#pragma region função das threads
unsigned WINAPI ThreadMonitorarEventosEscritaCLP(LPVOID arg) {
    monitorarEventosEscritaCLP();
    return 0;
}

unsigned WINAPI ThreadMonitorarEventosLeituraCLP(LPVOID arg) {
    monitorarEventosLeituraCLP();
    return 0;
}

unsigned WINAPI ThreadMonitorarEventosEscritaMES(LPVOID arg) {
    monitorarEventosEscritaMES();
    return 0;
}

unsigned WINAPI ThreadMonitorarEventosLeituraMES(LPVOID arg) {
    monitorarEventosLeituraMES();
    return 0;
}

unsigned WINAPI ThreadMonitorarEventosRetiradaDeMensagens(LPVOID arg) {
    monitorarEventosRetiradaDeMensagens();
    return 0;
}
#pragma endregion

int main() {
    try {
        bufferUnificado = new BufferCircular<MensagemUnificada, MAX_MESSAGES>();

        criarEventos();
        criarMutex();
        criarSemaforos();

        iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Leitura_Teclado.exe");
        iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Exibicao_de_Setups_de_Producao.exe");
        iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Exibicao_Dados_Processo.exe");

        unsigned threadID;
        HANDLE hThreadEscritaCLP = (HANDLE)_beginthreadex(
            NULL,
            0,
            (CAST_FUNCTION)ThreadMonitorarEventosEscritaCLP,
            NULL,
            0,
            (CAST_LPDWORD)&threadID
        );

        HANDLE hThreadLeituraCLP = (HANDLE)_beginthreadex(
            NULL,
            0,
            (CAST_FUNCTION)ThreadMonitorarEventosLeituraCLP,
            NULL,
            0,
            (CAST_LPDWORD)&threadID
        );

        HANDLE hThreadEscritaMES = (HANDLE)_beginthreadex(
            NULL,
            0,
            (CAST_FUNCTION)ThreadMonitorarEventosEscritaMES,
            NULL,
            0,
            (CAST_LPDWORD)&threadID
        );

        HANDLE hThreadLeituraMES = (HANDLE)_beginthreadex(
            NULL,
            0,
            (CAST_FUNCTION)ThreadMonitorarEventosLeituraMES,
            NULL,
            0,
            (CAST_LPDWORD)&threadID
        );

        HANDLE hThreadRetirada = (HANDLE)_beginthreadex(
            NULL,
            0,
            (CAST_FUNCTION)ThreadMonitorarEventosRetiradaDeMensagens,
            NULL,
            0,
            (CAST_LPDWORD)&threadID
        );

        if (hThreadEscritaCLP == NULL || hThreadLeituraCLP == NULL ||
            hThreadEscritaMES == NULL || hThreadLeituraMES == NULL ||
            hThreadRetirada == NULL) {
            throw std::runtime_error("Erro ao criar threads");
        }

        HANDLE threads[] = {
            hThreadEscritaCLP,
            hThreadLeituraCLP,
            hThreadEscritaMES,
            hThreadLeituraMES,
            hThreadRetirada
        };

        WaitForMultipleObjects(5, threads, TRUE, INFINITE);

        CloseHandle(hThreadEscritaCLP);
        CloseHandle(hThreadLeituraCLP);
        CloseHandle(hThreadEscritaMES);
        CloseHandle(hThreadLeituraMES);
        CloseHandle(hThreadRetirada);


        std::cout << "Sistema inicializado. Gerenciando tarefas..." << std::endl;
        gerenciarTarefas();
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
    }

    liberarRecursos();
    return 0;
}