
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
BufferCircular<Mensagem, MAX_MESSAGES>* lista_1;
BufferCircular<MensagemDeStatus, 100>* lista_22;


const wchar_t* mutexName = L"Global\\MeuMutexCompartilhado";
const wchar_t* SEMAPHORE_NAME = L"Global\\MeuSemaforo";

HANDLE hMutex;
HANDLE hSemaphore;
HANDLE hMutexLista1;
HANDLE hMutexLista2;
BufferCircular<MensagemDeStatus, 1000> statusBuffer;
BufferCircular<MensagemDeSetup, 1000> setupBuffer;

HANDLE hThreadMES;
DWORD WINAPI MESFunc(LPVOID);


const wchar_t* SHARED_MEMORY_LISTA1 = L"Global\\SharedBufferLista1";
const wchar_t* SHARED_MEMORY_LISTA2 = L"Global\\SharedBufferLista2";
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
BufferCircular<Mensagem, MAX_MESSAGES>* lista_mensagem_1;

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

void adicionarMensagemAoBuffer() {
    static int nseq = 0;

    MensagemDeStatus mensagem;

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

    WaitForSingleObject(hMutex, INFINITE);
    if (!statusBuffer.estaCheia()) {
        statusBuffer.adicionarMensagem(mensagem);
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

        MensagemDeStatus mensagem;
        WaitForSingleObject(hMutex, INFINITE);
        if (statusBuffer.recuperarMensagem(mensagem)) {
            std::cout << std::setfill('0')
                << std::setw(5) << mensagem.nseq << " | "
                << mensagem.linha << " | "
                << std::setw(8) << std::fixed << std::setprecision(1) << mensagem.prod_acum << " | "
                << std::setw(5) << mensagem.nivel_xar << " | "
                << std::setw(5) << mensagem.nivel_agua << " | "
                << std::setw(4) << mensagem.downtime << " | "
                << std::setw(2) << mensagem.timestamp.wHour << ":"
                << std::setw(2) << mensagem.timestamp.wMinute << ":"
                << std::setw(2) << mensagem.timestamp.wSecond << std::endl;
        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void adicionarMensagemAoBufferMES() {
    static int nseq = 0;

    MensagemDeSetup mensagem;

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

    WaitForSingleObject(hMutex, INFINITE);
    if (!setupBuffer.estaCheia()) {
        setupBuffer.adicionarMensagem(mensagem);
        std::cout << "Mensagem adicionada ao buffer: NSEQ=" << 0000 << std::endl;
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

        MensagemDeSetup mensagem_mes;

        WaitForSingleObject(hMutex, INFINITE);
        if (setupBuffer.recuperarMensagem(mensagem_mes)) {

            std::cout << "MES: " << mensagem_mes.nseq << " | "
                << "Produção Acumulada: " << mensagem_mes.sp_sep << " | "
                << "Nível de Xarope: " << mensagem_mes.sp_vel << " | "
                << "Nível de Água: " << mensagem_mes.sp_ench << " | "
                << "Timestamp: "
                << (mensagem_mes.timestamp.wHour < 10 ? "0" : "") << mensagem_mes.timestamp.wHour << ":"
                << (mensagem_mes.timestamp.wMinute < 10 ? "0" : "") << mensagem_mes.timestamp.wMinute << ":"
                << (mensagem_mes.timestamp.wSecond < 10 ? "0" : "") << mensagem_mes.timestamp.wSecond
                << std::endl;

        }
        ReleaseMutex(hMutex);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {

    DWORD dwThreadId;
    //Mutex compartilhado
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
    iniciarProcesso(L"..\\x64\\Debug\\Tarefa_Leitura_CLP.exe");
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