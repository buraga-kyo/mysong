// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROJECTOR — src/nucleo/video.hpp
// ══════════════════════════════════════════════════════════════════════════
// O VÍDEO em janella propria do systema, e NÃO dentro do terminal. A propria
// documentação do mpv diz que a sahida grafica d'elle não sincroniza com o resto
// do terminal; a decisão tomada foi janella á parte, com qualidade cheia.
//
// E é uma SEGUNDA instancia de mpv, em processo proprio, e não a libmpv que o
// motor já embute. Duas razões, e as duas pesadas:
//
// 1. o motor abre-se com «video=no» ANTES de mpv_initialize, e essa opção não se
//    volta atraz sem reabrir o punho: reabri-lo derrubaria o som que toca;
// 2. a tarefa pede janella com CLASSE propria, para o RADICAL-OS a governar por
//    regra depois, e pede que `pgrep` prove que processo algum fica para traz.
//    Processo proprio é o unico modo de as duas cousas serem verdade.
//
// O governo é pelo soquete de commandos do mpv, em JSON por linha. Tecla alguma se
// manda á janella: a TUI manda ordem pelo soquete, e a janella obedece.
//
// DOMÍNIO ......... o caminho de uma faixa, e as ordens do operador.
// CONTRA-DOMÍNIO .. uma janella do systema a tocar, e um processo que morre
//                   quando se manda, sem deixar orfão.
// INVARIANTE ...... o AUDIO não dobra. Quem cala o motor é quem abre a fita, e a
//                   fita nunca se abre com o motor a tocar.
// Q.E.D. .......... sendo puras a lista de argumentos, a redacção das ordens e o
//                   juizo de que ha video, a bateria afere o que se HA DE correr e
//                   o que se HA DE mandar sem abrir janella alguma.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <sys/types.h>

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// A CLASSE da janella. Fixa, e n'uma constante com nome: é por ella que o
// RADICAL-OS a governa por regra, e regra que aponte para nome que se mova é regra
// que se quebra calada.
inline constexpr char kClasseDoVideo[] = "mysong-video";

// tem_video — o juizo pela EXTENSÃO, e a lista é fechada. O limite vae declarado:
// um `.mkv` sem faixa de video existe, e este juizo diz que tem. Perguntar ao mpv
// seria certo e custaria um processo por faixa a cada quadro da tela.
bool tem_video(const std::filesystem::path& faixa);

// extensao_com_video — a mesma pergunta, feita á extensão sósinha. Existe porque a
// VARREDURA tambem precisa d'esta lista, e duas listas em dous arquivos dariam
// duas verdades: o dia em que uma ganhasse `.mkv` e a outra não, o acervo indexava
// o que a tela recusava abrir. A lista tem UM logar, e é este.
bool extensao_com_video(std::string_view extensao);

// caminho_do_soquete — `<raiz>/mysong-video-<pid>.sock`. O pid entra no nome para
// que duas corridas do mysong não disputem o mesmo soquete.
std::filesystem::path caminho_do_soquete(const std::filesystem::path& raiz,
                                         long pid);

// argumentos_do_projector — o que se corre. A classe e o titulo vão declarados, e o
// `--` fecha as opções para que faixa chamada `--algo` não vire opção.
std::vector<std::string> argumentos_do_projector(
    const std::filesystem::path& faixa, const std::filesystem::path& soquete);

// escapa_json — as tres cousas que uma cadeia JSON não pode ter crús: a aspa, a
// barra invertida e o controle. Nome de propriedade é nosso, mas caminho de faixa
// vem do disco, e disco tras nome com aspa.
std::string escapa_json(std::string_view crua);

// As quatro redacções de ordem. Separadas pela FÓRMA do valor, e não uma que
// receba texto: `pause` quer `true` sem aspas, e `seek` quer numero. Uma só
// obrigaria quem chama a escrever o JSON, que é justamente o que se quer provar.
std::string ordem_simples(std::string_view verbo);
std::string ordem_de_bandeira(std::string_view propriedade, bool ligada);
std::string ordem_de_numero(std::string_view propriedade, double valor);
std::string ordem_de_busca(double segundos);

// ordem_de_busca_relativa — o deslocamento, e não a posição. Existe porque da
// janella não se sabe a posição sem lhe perguntar pelo soquete e esperar resposta,
// e o que a tecla do operador quer dizer é «cinco segundos adeante».
std::string ordem_de_busca_relativa(double deslocamento);

// ── E AGORA O QUE TOCA O MUNDO ──────────────────────────────────────────────

// O DESFECHO de abrir a fita. Toda falha tem nome, porque «não abriu» não diz ao
// operador se ha de installar o mpv, eleger outra faixa, ou olhar o écran.
enum class Fita {
  Rodando,        // a janella está de pé, e o soquete responde
  SemMpv,         // o binario do mpv não está no caminho
  SemVideo,       // a faixa não tem video: nada se abriu
  SemSoquete,     // o processo subiu, mas o soquete nunca respondeu
  NaoAbriu,       // nem se pôde erguer o processo
};

std::string_view razao_da_fita(Fita fita);

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
