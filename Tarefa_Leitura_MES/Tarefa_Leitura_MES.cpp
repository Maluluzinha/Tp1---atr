int main() {}
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
////struct MensagemDeStatus {
////    int nseq;
////    float sp_vel;
////    float sp_ench;
////    float sp_sep;
////    SYSTEMTIME timestamp;
////};
//
//float gerarValorReal(float min, float max) {
//    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
//}
//
//int gerarValorInteiro(int min, int max) {
//    return min + rand() % (max - min + 1);
//}
//
//void mesReadFunction() {
//    const int MAX_NSEQ = 99999;
//    int nseq = 0;
//
//    HANDLE mesReadEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"MESReadEvent");
//    HANDLE terminateEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"TerminateEvent");
//
//    if (!mesReadEvent || !terminateEvent) {
//        std::cerr << "Erro ao abrir eventos." << std::endl;
//        return;
//    }
//
//    while (true) {
//        if (WaitForSingleObject(terminateEvent, 0) == WAIT_OBJECT_0) {
//            break;
//        }
//        WaitForSingleObject(mesReadEvent, INFINITE);
//
//        nseq = (nseq + 1) % (MAX_NSEQ + 1);
//
//        MensagemDeSetup mensagem;
//        mensagem.nseq = nseq;
//        mensagem.sp_sep = gerarValorReal(0.0f, 999999.9f);
//        mensagem.sp_vel = gerarValorReal(0.0f, 999.9f);
//        mensagem.sp_ench = gerarValorReal(0.0f, 999.9f);
//        GetSystemTime(&mensagem.timestamp);
//
//        std::cout << "MES: " << mensagem.nseq << " | "
//            << "Produção Acumulada: " << mensagem.sp_sep << " | "
//            << "Nível de Xarope: " << mensagem.sp_vel << " | "
//            << "Nível de Água: " << mensagem.sp_ench << " | "
//            << "Timestamp: "
//            << (mensagem.timestamp.wHour < 10 ? "0" : "") << mensagem.timestamp.wHour << ":"
//            << (mensagem.timestamp.wMinute < 10 ? "0" : "") << mensagem.timestamp.wMinute << ":"
//            << (mensagem.timestamp.wSecond < 10 ? "0" : "") << mensagem.timestamp.wSecond
//            << std::endl;
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    }
//
//    std::cout << "[MES] Processo encerrado." << std::endl;
//}
//
//int main() {
//    mesReadFunction();
//    return 0;
//}
