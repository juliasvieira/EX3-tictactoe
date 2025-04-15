#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>
#include <windows.h> 

// classe TicTacToe--------------------------------------------------------:
class TicTacToe {
private:
    std::vector<std::vector<char>> tabuleiro; // Matriz 3x3 para o tabuleiro
    std::mutex board_mutex; // Mutex para proteger acesso ao tabuleiro
    std::condition_variable turn_cv;    // Condition variable para alternar turnos
    char jogador_atual; // 'X' ou 'O' indica de quem é a vez
    bool jogo_terminado;    // Flag para estado do jogo
    char vencedor;                            // 'X', 'O' ou 'D' (empate)

public:
// Construtor que inicializa tabuleiro vazio, jogador X começa, jogo não terminado
    
    TicTacToe() : tabuleiro(3, std::vector<char>(3, ' ')), jogador_atual('X'),
                  jogo_terminado(false), vencedor(' ') {}

    void exibir_tabuleiro() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));   // Pausa para ver melhor
        system("cls"); // limpa a tela p impressao dinamica

        std::lock_guard<std::mutex> lock(board_mutex);  // Bloqueia acesso ao tabuleiro

            // Imprime o tabuleiro linha por linha
        for (int i = 0; i < 3; ++i) {
            std::cout << " ";
            for (int j = 0; j < 3; ++j) {
                std::cout << tabuleiro[i][j];
                if (j < 2) std::cout << " | ";  // Separador de colunas
            }

            std::cout << "\n";
            if (i < 2) {
                std::cout << "-----------\n";   // Linha entre linhas
            }
        }

        char aux_jogador = (jogador_atual == 'X') ? 'O' : 'X';  // Mostra jogador da próxima jogada

        // Mensagem de status do jogo
        if (!jogo_terminado) {
            std::cout << "\nCurrent player: " << aux_jogador << "\n";
        } else {
            std::cout << "\nCurrent player: " << jogador_atual << "\n\n";
        }
    }

    bool fazer_jogada(char jogador, int linha, int coluna) {
        std::unique_lock<std::mutex> lock(board_mutex); // Lock único para condition variable

        // Espera enquanto não for o turno do jogador ou o jogo terminou
        while (jogador_atual != jogador && !jogo_terminado) {
            turn_cv.wait(lock); // Libera o lock e espera notificação
        }

        if (jogo_terminado) return false;   // Se jogo acabou, retorna falha

        // Verifica se a jogada é válida (dentro dos limites e posição vazia)
        if (linha < 0 || linha >= 3 || coluna < 0 || coluna >= 3 || tabuleiro[linha][coluna] != ' ') {
            return false;
        }

        tabuleiro[linha][coluna] = jogador; // Executa a jogada

        // Verifica as condições de término
        if (checar_vitoria(jogador)) {
            jogo_terminado = true;
            vencedor = jogador;
        } else if (checar_empate()) {
            jogo_terminado = true;
            vencedor = 'D'; // Empate (Draw)
        }

        lock.unlock();  // Libera o lock para exibir o tabuleiro
        exibir_tabuleiro(); // Mostra o novo estado
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));   // Pausa para visualização

        lock.lock();    // Relock para modificar jogador_atual
        jogador_atual = (jogador_atual == 'X') ? 'O' : 'X'; // Alterna turno
        lock.unlock();
        turn_cv.notify_all();   // Notifica todas as threads para verificar turnos

        return true;    // Jogada bem sucedida
    }

    bool checar_vitoria(char jogador) {
        for (int i = 0; i < 3; ++i) {                   //verifica se jogador completou linha na vertical
            if (tabuleiro[i][0] == jogador && tabuleiro[i][1] == jogador && tabuleiro[i][2] == jogador) {
                return true;
            }
        }

        for (int j = 0; j < 3; ++j) {                  //verifica se jogador completou linha na horizontal
            if (tabuleiro[0][j] == jogador && tabuleiro[1][j] == jogador && tabuleiro[2][j] == jogador) {
                return true;
            }
        }

        //verifica se jogador completou linha nas diagonais
        if (tabuleiro[0][0] == jogador && tabuleiro[1][1] == jogador && tabuleiro[2][2] == jogador) {
            return true;
        }
        if (tabuleiro[0][2] == jogador && tabuleiro[1][1] == jogador && tabuleiro[2][0] == jogador) {
            return true;
        }

        return false;
    }

    bool checar_empate() {
        // Verifica se todas as posições estão preenchidas
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (tabuleiro[i][j] == ' ') {
                    return false;   // Ainda há espaços vazios
                }
            }
        }
        return true;    // Tabuleiro completo (empate)
    }

    // Métodos de acesso
    bool is_game_over() const { return jogo_terminado; }
    char get_winner() const { return vencedor; }
};

// classe Player--------------------------------------------------------:
class Player {
private:
    char simbolo;   // 'X' ou 'O'
    std::string estrategia; // "sequencial" ou "aleatório"

public:
// Construtor
    TicTacToe& jogo;    // Referência ao jogo principal

    Player(TicTacToe& g, char s, const std::string& strat)
        : jogo(g), simbolo(s), estrategia(strat) {}

    void play() {
        if (estrategia == "sequencial") {
            play_sequential();  // Usa estratégia sequencial
        } else {
            play_random();  // Usa estratégia aleatória
        }
    }

    void play_sequential() {
        // Percorre o tabuleiro em ordem até encontrar posição vazia
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (jogo.fazer_jogada(simbolo, i, j)) { // Tenta jogar
                    return; // Sai após primeira jogada válida
                }
            }
        }
    }

    void play_random() {
        std::random_device rd;  // Dispositivo de aleatoriedade
        std::mt19937 gen(rd()); // Gerador Mersenne Twister
        std::uniform_int_distribution<> dis(0, 2);  // Distribuição uniforme 0-2

        // Tenta jogadas aleatórias até conseguir
        while (!jogo.is_game_over()) {
            int linha = dis(gen);   // Gera linha aleatória
            int coluna = dis(gen);  // Gera coluna aleatória
            if (jogo.fazer_jogada(simbolo, linha, coluna)) {
                return; // Sai após jogada válida
            }
        }
    }
};

void player_thread_function(Player* jogador) {
    // Executa jogadas até o jogo terminar
    while (!jogador->jogo.is_game_over()) {
        jogador->play();
    }
}

// main--------------------------------------------------------------------:
int main() {
    TicTacToe jogo; // Cria instância do jogo

    // Cria jogadores com estratégias diferentes
    Player jogador1(jogo, 'X', "sequencial");
    Player jogador2(jogo, 'O', "aleatório");

    // Inicia threads para cada jogador
    std::thread t1(player_thread_function, &jogador1);
    std::thread t2(player_thread_function, &jogador2);

    // Loop de espera pelo término do jogo
    while (!jogo.is_game_over()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // loop de espera do fim das threads
    }

    jogo.exibir_tabuleiro(); // Exibe estado final

    // Mostra resultado
    char vencedor = jogo.get_winner();
    if (vencedor == 'D') {
        std::cout << "The game ended in a tie!\n";
    } else {
        std::cout << "Player " << vencedor << " wins!\n\n";
    }

    // Espera threads terminarem
    t1.join();
    t2.join();

    return 0;
}
