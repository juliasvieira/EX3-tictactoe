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
    std::vector<std::vector<char>> tabuleiro;
    std::mutex board_mutex;
    std::condition_variable turn_cv;
    char jogador_atual;
    bool jogo_terminado;
    char vencedor;

public:
    TicTacToe() : tabuleiro(3, std::vector<char>(3, ' ')), jogador_atual('X'),
                  jogo_terminado(false), vencedor(' ') {}

    void exibir_tabuleiro() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        system("cls"); // limpa a tela p impressao dinamica

        std::lock_guard<std::mutex> lock(board_mutex);

        for (int i = 0; i < 3; ++i) {
            std::cout << " ";
            for (int j = 0; j < 3; ++j) {
                std::cout << tabuleiro[i][j];
                if (j < 2) std::cout << " | ";
            }

            std::cout << "\n";
            if (i < 2) {
                std::cout << "-----------\n";
            }
        }

        char aux_jogador = (jogador_atual == 'X') ? 'O' : 'X';

        if (!jogo_terminado) {
            std::cout << "\nCurrent player: " << aux_jogador << "\n";
        } else {
            std::cout << "\nCurrent player: " << jogador_atual << "\n\n";
        }
    }

    bool fazer_jogada(char jogador, int linha, int coluna) {
        std::unique_lock<std::mutex> lock(board_mutex);

        while (jogador_atual != jogador && !jogo_terminado) {
            turn_cv.wait(lock);
        }

        if (jogo_terminado) return false;

        if (linha < 0 || linha >= 3 || coluna < 0 || coluna >= 3 || tabuleiro[linha][coluna] != ' ') {
            return false;
        }

        tabuleiro[linha][coluna] = jogador;

        if (checar_vitoria(jogador)) {
            jogo_terminado = true;
            vencedor = jogador;
        } else if (checar_empate()) {
            jogo_terminado = true;
            vencedor = 'D';
        }

        lock.unlock();
        exibir_tabuleiro();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        lock.lock();
        jogador_atual = (jogador_atual == 'X') ? 'O' : 'X';
        lock.unlock();
        turn_cv.notify_all();

        return true;
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
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (tabuleiro[i][j] == ' ') {
                    return false;
                }
            }
        }
        return true;
    }

    bool is_game_over() const { return jogo_terminado; }
    char get_winner() const { return vencedor; }
};

// classe Player--------------------------------------------------------:
class Player {
private:
    char simbolo;
    std::string estrategia;

public:
    TicTacToe& jogo;

    Player(TicTacToe& g, char s, const std::string& strat)
        : jogo(g), simbolo(s), estrategia(strat) {}

    void play() {
        if (estrategia == "sequencial") {
            play_sequential();
        } else {
            play_random();
        }
    }

    void play_sequential() {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (jogo.fazer_jogada(simbolo, i, j)) {
                    return;
                }
            }
        }
    }

    void play_random() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 2);

        while (!jogo.is_game_over()) {
            int linha = dis(gen);
            int coluna = dis(gen);
            if (jogo.fazer_jogada(simbolo, linha, coluna)) {
                return;
            }
        }
    }
};

void player_thread_function(Player* jogador) {
    while (!jogador->jogo.is_game_over()) {
        jogador->play();
    }
}

// main--------------------------------------------------------------------:
int main() {
    TicTacToe jogo;

    Player jogador1(jogo, 'X', "sequencial");
    Player jogador2(jogo, 'O', "aleatório");

    std::thread t1(player_thread_function, &jogador1);
    std::thread t2(player_thread_function, &jogador2);

    while (!jogo.is_game_over()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // loop de espera do fim das threads
    }

    jogo.exibir_tabuleiro();

    char vencedor = jogo.get_winner();
    if (vencedor == 'D') {
        std::cout << "The game ended in a tie!\n";
    } else {
        std::cout << "Player " << vencedor << " wins!\n\n";
    }

    t1.join();
    t2.join();

    return 0;
}
