#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>

const wchar_t* pipeName = L"\\\\.\\pipe\\MeuPipe";

void conectarPipeEExibirDados() {
    HANDLE hPipe = CreateFile(
        pipeName,               // Nome da pipe
        GENERIC_READ,           // Acesso de leitura
        0,                      // Sem compartilhamento
        NULL,                   // Segurança padrão
        OPEN_EXISTING,          // Abre a pipe existente
        0,                      // Atributos padrão
        NULL);                  // Sem modelo de atributo

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Erro ao conectar à pipe. Código: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "Conexão à pipe estabelecida." << std::endl;

    while (true) {
        char buffer[512];
        DWORD bytesRead;

        // Tenta ler dados da pipe
        if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0'; // Garante que o buffer seja terminado com '\0'

            std::cout << "Mensagem recebida da pipe: " << buffer << std::endl;

            // Formata e exibe os dados
            std::cout << "HH:MM:SS " << buffer << std::endl;
        }
        else {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                std::cout << "Conexão com a pipe foi encerrada pelo servidor." << std::endl;
                break;
            }
            else {
                std::cerr << "Erro ao ler da pipe. Código: " << error << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Aguarda antes de tentar novamente
    }

    CloseHandle(hPipe);
}

int main() {
    std::cout << "Cliente de leitura iniciado..." << std::endl;

    conectarPipeEExibirDados();

    return 0;
}
