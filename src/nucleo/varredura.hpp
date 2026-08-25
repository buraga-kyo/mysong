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

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
