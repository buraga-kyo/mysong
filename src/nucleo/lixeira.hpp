// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LIXEIRA — src/nucleo/lixeira.hpp
// ══════════════════════════════════════════════════════════════════════════
// Apagar uma faixa é MANDÁ-LA Á LIXEIRA, e nunca desligá-la do disco: o que se
// apaga por engano volta pela porta do gerenciador de arquivos. Segue-se a
// especificação freedesktop, que é a que elle já lê.
//
// DOMÍNIO ......... um caminho de arquivo, e a raiz da lixeira, POR PARÂMETRO.
// CONTRA-DOMÍNIO .. o arquivo em `Trash/files/<nome>`, o bilhete em
//                   `Trash/info/<nome>.trashinfo`, e o desfecho por escripto.
// INVARIANTE ...... bilhete e arquivo têm o MESMO nome, e o bilhete nasce ANTES
//                   da mudança: arquivo sem bilhete não se restaura. Collisão
//                   resolve-se por suffixo nos DOUS.
// Q.E.D. .......... entrando a raiz por parâmetro, a bateria corre em
//                   temporario, e prova alguma toca a lixeira de quem nos usa.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <ctime>
#include <filesystem>
#include <string>
#include <string_view>

namespace mysong::nucleo {

// O DESFECHO. A falha tem razão escripta: «não deu» não diz á tela se ha de
// avisar o operador ou calar-se.
struct DaLixeira {
  bool feita = false;
  bool copiada = false;        // outro volume: copiou-se e apagou-se
  bool levou_a_letra = false;  // o `.lrc` ao lado foi junto
  std::string nome;            // o nome que ficou em `Trash/files`
  std::string razao;           // vazio quando feita
};

// O percent-encoding do `Path=` do bilhete, e a data local em ISO 8601. Sahem
// aqui, e não escondidas na lavra, porque são as duas regras da especificação
// que se afferem contra alvo escripto á mão, sem disco algum. CAMINHO, e não
// `escapa_para_url` da letra: aquella escapa a barra, que é o que um parametro
// de consulta pede, e escapada aqui a lixeira não restaurava faixa alguma.
std::string escapa_o_caminho(std::string_view cru);
std::string data_da_exclusao(std::time_t quando);

// `$XDG_DATA_HOME/Trash`, e sem a variavel `~/.local/share/Trash`.
std::filesystem::path caminho_da_lixeira();

// O arquivo, e o `.lrc` ao lado havendo-o. A raiz entra por parâmetro: é por
// ella que a bateria desvia a lixeira. A curta toma a do systema.
DaLixeira manda_a_lixeira(const std::filesystem::path& caminho,
                          const std::filesystem::path& lixeira);
DaLixeira manda_a_lixeira(const std::filesystem::path& caminho);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
