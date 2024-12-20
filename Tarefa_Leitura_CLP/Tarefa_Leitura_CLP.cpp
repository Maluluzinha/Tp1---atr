//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <iostream>
//#include <thread>
//#include <chrono>
//#include <atomic>
//#include <iomanip>
//#include <sstream>
//#include <cstdlib>
//
//#include "../Tp_parte_inicial/buffer_circular.h"
//
//BufferCircular<MensagemDeStatus, 1000> statusBuffer;
//HANDLE hMutex;
//HANDLE clpReadEvent;
//HANDLE escEvent;
//HANDLE semEscritoras;
//HANDLE semLeitoras;
//std::atomic<bool> continuarProcesso{ true };
//
//void criarEventos() {
//    clpReadEvent = CreateEvent(NULL, TRUE, FALSE, L"ClpReadEvent");
//    escEvent = CreateEvent(NULL, TRUE, FALSE, L"EscEvent");
//
//    if (!clpReadEvent || !escEvent) {
//        std::cerr << "Erro ao criar eventos. Código: " << GetLastError() << std::endl;
//        exit(EXIT_FAILURE);
//    }
//}
//
//void liberarEventos() {
//    CloseHandle(clpReadEvent);
//    CloseHandle(escEvent);
//}
//
//void criarMutex() {
//    hMutex = CreateMutex(NULL, FALSE, L"Global\MeuMutexCompartilhado");
//    if (hMutex == NULL) {
//        std::cerr << "Erro ao criar o mutex. Código: " << GetLastError() << std::endl;
//        exit(EXIT_FAILURE);
//    }
//}
//
//void liberarMutex() {
//    CloseHandle(hMutex);
//}
//
//void criarSemaforos() {
//    semEscritoras = CreateSemaphore(NULL, 0, 5, NULL);
//    semLeitoras = CreateSemaphore(NULL, 0, 1, NULL);
//
//    if (!semEscritoras || !semLeitoras) {
//        std::cerr << "Erro na criação dos semáforos" << std::endl;
//        exit(EXIT_FAILURE);
//    }
//}
//
//void liberarSemaforos() {
//    CloseHandle(semEscritoras);
//    CloseHandle(semLeitoras);
//}
//
//void adicionarMensagemAoBuffer() {
//    static int nseq = 0;
//
//    MensagemDeStatus mensagem;
//
//    mensagem.nseq = nseq++;
//    if (nseq > 99999) nseq = 0;
//
//    mensagem.linha = 1 + (rand() % 2);
//
//    {
//        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
//        std::stringstream ss;
//        ss << std::fixed << std::setprecision(2) << std::setw(8) << std::setfill('0') << valor;
//        mensagem.prod_acum = std::stof(ss.str());
//    }
//
//    {
//        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
//        std::stringstream ss;
//        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
//        mensagem.nivel_xar = std::stof(ss.str());
//    }
//
//    {
//        float valor = static_cast<float>(rand() % 1000) + static_cast<float>(rand() % 100) / 100.0f;
//        std::stringstream ss;
//        ss << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << valor;
//        mensagem.nivel_agua = std::stof(ss.str());
//    }
//
//    mensagem.downtime = rand() % 10000;
//
//    GetLocalTime(&mensagem.timestamp);
//
//    WaitForSingleObject(hMutex, INFINITE);
//    if (!statusBuffer.estaCheia()) {
//        statusBuffer.adicionarMensagem(mensagem);
//        std::cout << "Mensagem adicionada ao buffer: NSEQ=" << mensagem.downtime << std::endl;
//    }
//    ReleaseMutex(hMutex);
//
//    ReleaseSemaphore(semEscritoras, 1, NULL);
//}
//
//void monitorarEventosEscrita() {
//    bool bloqueado = false;
//
//    while (continuarProcesso) {
//        HANDLE eventos[] = { clpReadEvent, escEvent };
//        DWORD waitResult = WaitForMultipleObjects(2, eventos, FALSE, 0);
//
//        if (waitResult == WAIT_OBJECT_0) {
//            bloqueado = !bloqueado;
//            std::cout << (bloqueado ? "Processo bloqueado" : "Processo desbloqueado") << std::endl;
//            ResetEvent(clpReadEvent);
//        }
//        else if (waitResult == WAIT_OBJECT_0 + 1) {
//            std::cout << "Encerrando processo..." << std::endl;
//            continuarProcesso = false;
//            exit(0);
//        }
//
//        if (!bloqueado) {
//            adicionarMensagemAoBuffer();
//            std::this_thread::sleep_for(std::chrono::seconds(1));
//        }
//    }
//}
//
//void monitorarEventosLeitura() {
//    while (continuarProcesso) {
//        HANDLE eventos[] = { escEvent };
//        DWORD waitResult = WaitForMultipleObjects(1, eventos, FALSE, 0);
//
//        if (waitResult == WAIT_OBJECT_0) {
//            std::cout << "Encerrando processo..." << std::endl;
//            continuarProcesso = false;
//            exit(0);
//        }
//
//        MensagemDeStatus mensagem;
//        WaitForSingleObject(hMutex, INFINITE);
//        if (statusBuffer.recuperarMensagem(mensagem)) {
//            std::cout << std::setfill('0')
//                << std::setw(5) << mensagem.nseq << " | "
//                << mensagem.linha << " | "
//                << std::setw(8) << std::fixed << std::setprecision(1) << mensagem.prod_acum << " | "
//                << std::setw(5) << mensagem.nivel_xar << " | "
//                << std::setw(5) << mensagem.nivel_agua << " | "
//                << std::setw(4) << mensagem.downtime << " | "
//                << std::setw(2) << mensagem.timestamp.wHour << ":"
//                << std::setw(2) << mensagem.timestamp.wMinute << ":"
//                << std::setw(2) << mensagem.timestamp.wSecond << std::endl;
//        }
//        ReleaseMutex(hMutex);
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    }
//}
//
//int main() {
//    criarEventos();
//    criarMutex();
//    criarSemaforos();
//
//    std::thread threadEscrita(monitorarEventosEscrita);
//    std::thread threadLeitura(monitorarEventosLeitura);
//
//    threadEscrita.join();
//    threadLeitura.join();
//
//    liberarEventos();
//    liberarMutex();
//    liberarSemaforos();
//
//    return 0;
//}

int main() {}