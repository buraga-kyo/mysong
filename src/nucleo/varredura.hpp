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

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
