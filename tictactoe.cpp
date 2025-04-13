#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

class TicTacToe {
private:
    // Atributos conforme especificação
    std::vector<std::vector<char>> tabuleiro;  // Matriz 3x3 de caracteres
    std::mutex board_mutex;                   // Mutex para controlar acesso ao tabuleiro
    std::condition_variable turn_cv;          // Variável de condição para coordenar os turnos
    char jogador_atual;                       // 'X' ou 'O' indicando o jogador atual
    bool jogo_terminado;                      // Booleano indicando se o jogo acabou
    char vencedor;                            // 'X', 'O' ou 'D' (empate)

public:
    TicTacToe() : tabuleiro(3, std::vector<char>(3, ' ')), jogador_atual('X'), 
                 jogo_terminado(false), vencedor(' ') {}

    void exibir_tabuleiro() {
        std::lock_guard<std::mutex> lock(board_mutex);
        for (int i = 0; i < 3; ++i) {
            std::cout << "|";
            for (int j = 0; j < 3; ++j) {
                std::cout << "  " << tabuleiro[i][j] << "  |";
            }
            std::cout << "\n--------------\n";
        }
        std::cout << "Jogador atual: " << jogador_atual << "\n\n";
    }

    bool fazer_jogada(char jogador, int linha, int coluna) {
        std::unique_lock<std::mutex> lock(board_mutex);
        
        // Espera pela vez do jogador
        turn_cv.wait(lock, [this, jogador]() { 
            return jogador_atual == jogador || jogo_terminado; 
        });

        if (jogo_terminado) return false;

        // Verifica se a jogada é válida
        if (linha < 0 || linha >= 3 || coluna < 0 || coluna >= 3 || tabuleiro[linha][coluna] != ' ') {
            return false;
        }

        // Executa a jogada
        tabuleiro[linha][coluna] = jogador;
        
        // Verifica vitória ou empate
        if (checar_vitoria(jogador)) {
            jogo_terminado = true;
            vencedor = jogador;
        } else if (checar_empate()) {
            jogo_terminado = true;
            vencedor = 'D';
        }

        // Alterna o jogador
        jogador_atual = (jogador_atual == 'X') ? 'O' : 'X';
        
        // Notifica o outro jogador
        lock.unlock();
        turn_cv.notify_all();

        return true;
    }

    bool checar_vitoria(char jogador) {
        // Verifica linhas
        for (int i = 0; i < 3; ++i) {
            if (tabuleiro[i][0] == jogador && tabuleiro[i][1] == jogador && tabuleiro[i][2] == jogador) {
                return true;
            }
        }

        // Verifica colunas
        for (int j = 0; j < 3; ++j) {
            if (tabuleiro[0][j] == jogador && tabuleiro[1][j] == jogador && tabuleiro[2][j] == jogador) {
                return true;
            }
        }

        // Verifica diagonais
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
    char get_jogador_atual() const { return jogador_atual; }
};

class Player {
private:
    // Atributos conforme especificação
    TicTacToe& jogo;         // Referência para o objeto TicTacToe
    char simbolo;            // Símbolo do jogador ('X' ou 'O')
    std::string estrategia;  // Estratégia do jogador ('sequencial' ou 'aleatório')

public:
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
        // Pequeno delay para melhor visualização
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    TicTacToe jogo;

    // Cria jogadores com estratégias diferentes
    Player jogador1(jogo, 'X', "sequencial");
    Player jogador2(jogo, 'O', "aleatório");

    // Inicia as threads dos jogadores
    std::thread t1(player_thread_function, &jogador1);
    std::thread t2(player_thread_function, &jogador2);

    // Exibe o estado do jogo periodicamente
    while (!jogo.is_game_over()) {
        jogo.exibir_tabuleiro();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Exibe o tabuleiro final
    jogo.exibir_tabuleiro();

    // Mostra o resultado
    char vencedor = jogo.get_winner();
    if (vencedor == 'D') {
        std::cout << "Jogo terminou em empate!\n";
    } else {
        std::cout << "Jogador " << vencedor << " venceu!\n";
    }

    // Aguarda as threads terminarem
    t1.join();
    t2.join();

    return 0;
}