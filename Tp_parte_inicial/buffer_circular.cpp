#include "buffer_circular.h"
#include <mutex>
#include <condition_variable>
#include <array>

std::array<MensagemDeSetup, MAX_SETUP_MESSAGES> setupMessages;
size_t setupHead = 0;
size_t setupTail = 0;
std::mutex setupMutex;
std::condition_variable setupCV;

void adicionarMensagemSetup(const MensagemDeSetup& mensagem) {
    std::unique_lock<std::mutex> lock(setupMutex);

    if ((setupTail + 1) % MAX_SETUP_MESSAGES == setupHead) {
        setupHead = (setupHead + 1) % MAX_SETUP_MESSAGES;
    }

    setupMessages[setupTail] = mensagem;
    setupTail = (setupTail + 1) % MAX_SETUP_MESSAGES;

    setupCV.notify_one();
}

bool recuperarMensagemSetup(MensagemDeSetup& mensagem) {
    std::unique_lock<std::mutex> lock(setupMutex);

    if (setupHead == setupTail) {
        return false;
    }

    mensagem = setupMessages[setupHead];
    setupHead = (setupHead + 1) % MAX_SETUP_MESSAGES;

    return true;
}

std::array<MensagemDeStatus, MAX_STATUS_MESSAGES> statusMessages;
size_t statusHead = 0;
size_t statusTail = 0;
std::mutex statusMutex;
std::condition_variable statusCV;

void adicionarMensagemStatus(const MensagemDeStatus& mensagem) {
    std::unique_lock<std::mutex> lock(statusMutex);

    if ((statusTail + 1) % MAX_STATUS_MESSAGES == statusHead) {
        statusHead = (statusHead + 1) % MAX_STATUS_MESSAGES;
    }

    statusMessages[statusTail] = mensagem;
    statusTail = (statusTail + 1) % MAX_STATUS_MESSAGES;

    statusCV.notify_one();
}

bool recuperarMensagemStatus(MensagemDeStatus& mensagem) {
    std::unique_lock<std::mutex> lock(statusMutex);

    if (statusHead == statusTail) {
        return false;
    }

    mensagem = statusMessages[statusHead];
    statusHead = (statusHead + 1) % MAX_STATUS_MESSAGES;

    return true;
}