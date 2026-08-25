// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA VARREDURA — src/nucleo/varredura.hpp
// ══════════════════════════════════════════════════════════════════════════
// Quem percorre o disco. Lê as etiquetas com taglib, deduz do caminho o que a
// etiqueta calar, e entrega as faixas ao Escriba da bibliotheca. Não sabe de
// SQLite: sabe de directorios e de etiquetas, e a bibliotheca sabe do resto.
//
// DOMÍNIO ......... uma ou mais raízes de acervo, e o caminho do banco. Os dous
//                   entram por PARÂMETRO, e é o que permitte á bateria correr
//                   sem tocar o índice do operador.
// CONTRA-DOMÍNIO .. um índice novo no logar, e um Progresso que conta o que se
//                   fez: lidas, reaproveitadas, recusadas, desaparecidas.
// INVARIANTE ...... conduz-se por PASSOS, e passo algum engole o acervo
//                   inteiro: quem chama fica dono do seu proprio relogio, e a
//                   tela não congela enquanto o disco se varre. O arquivo de
//                   destino NÃO existe antes do ultimo passo, e abandonar a
//                   meio conserva o índice anterior INTEIRO.
// Q.E.D. .......... vivendo o incremental na ETIQUETA e não no banco, o rename
//                   de um golpe e a varredura incremental deixam de se excluir:
//                   toda corrida reconstroe o índice, mas o arquivo cuja hora e
//                   cujo tamanho não mudaram tem a sua linha COPIADA da antiga
//                   em vez de relida, que é onde o primeiro scan gasta o tempo.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "nucleo/biblioteca.hpp"

namespace mysong::nucleo {

// O PROGRESSO da corrida. Cada campo existe para uma pergunta que o operador ou
// a prova ha de fazer, e nenhum é ornamento: `lidas` contra `reaproveitadas` é
// como se prova que a segunda corrida não releu etiqueta alguma.
struct Progresso {
  std::size_t vistas = 0;            // arquivos considerados
  std::size_t lidas = 0;             // etiquetas abertas por taglib
  std::size_t reaproveitadas = 0;    // linhas copiadas do índice antigo
  std::size_t recusadas = 0;         // extensão alheia, ou não é audio
  std::size_t desaparecidas = 0;     // sumiu entre listar e ler
  std::size_t ligacoes_saltadas = 0; // directorio symbólico, que não se desce
  std::size_t raizes_falhadas = 0;   // ausente, ou sem permissão
};

// deriva_do_caminho — o que o caminho diz de uma faixa. Funcção pura, e por isso
// aferivel contra alvo escripto á mão sem disco algum. O artista é o componente
// logo sob a raiz; o album é o directorio que contem o arquivo; e do nome do
// arquivo tira-se `NN - Titulo`. Arquivo directamente na raiz não tem artista
// nem album, e os dous bits acendem-se.
//
// Devolve a faixa com sómente estes quatro campos postos, e a máscara `deduzido`
// a nomear TODOS elles: quem lê a etiqueta depois apaga o bit do que a etiqueta
// disser. Assim a dedução é o piso, e a etiqueta o que se lhe sobrepõe.
Faixa deriva_do_caminho(const std::filesystem::path& caminho,
                        const std::filesystem::path& raiz);

// extensao_de_audio — se a extensão é das que se offerecem á taglib. Filtra-se
// ANTES de abrir: offerecer todo arquivo á taglib faria a varredura abrir cada
// PNG e cada texto do acervo para nada.
bool extensao_de_audio(std::string_view extensao);

// A VARREDURA. Conduz-se por passos, e nasce com o Escriba já aberto sobre um
// temporario: donde o destino não existe até se concluir, e o destructor desfaz
// o temporario de quem a abandonou.
class Varredura {
 public:
  Varredura(std::filesystem::path banco,
            std::vector<std::filesystem::path> raizes);
  ~Varredura();

  Varredura(const Varredura&) = delete;
  Varredura& operator=(const Varredura&) = delete;

  // Um passo. FALSO quando já não ha o que fazer, e ahi o desfecho diz o que
  // houve. Um passo trata UM arquivo, ou lista UMA raiz, ou conclue: nunca o
  // acervo inteiro, que é a razão de a tela não congelar.
  bool passo();

  // Desiste. O temporario desfaz-se, e o índice anterior fica intacto.
  void abandona();

  Desfecho desfecho() const noexcept;
  const Progresso& progresso() const noexcept;

 private:
  struct Punho;
  std::unique_ptr<Punho> punho_;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
