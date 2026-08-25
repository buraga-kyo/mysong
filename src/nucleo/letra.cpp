// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA — src/nucleo/letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o libcurl no fim, e sozinho.
//
// DOMÍNIO ......... o que se sabe da faixa, e o corpo do LRCLIB.
// CONTRA-DOMÍNIO .. um `.lrc` ao lado do audio, ou nada.
// INVARIANTE ...... funcção alguma d'aqui lança, e letra ausente não é falha.
// Q.E.D. .......... a rede toca-se n'uma funcção só, e por isso a bateria julga
//                   tudo o mais sobre corpos escriptos á mão.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/letra.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>

namespace mysong::nucleo {

std::string escapa_para_url(std::string_view crua) {
  static const char kCifras[] = "0123456789ABCDEF";
  std::string obra;
  obra.reserve(crua.size() * 3);
  for (const unsigned char octeto : crua) {
    // A lista do que NÃO se escapa é a do RFC 3986 para «unreserved», e é
    // fechada: escapar de mais é sempre seguro, e escapar de menos parte a
    // consulta no primeiro `&` que um titulo traga.
    const bool livre = std::isalnum(octeto) != 0 || octeto == '-' ||
                       octeto == '.' || octeto == '_' || octeto == '~';
    if (livre) {
      obra += static_cast<char>(octeto);
      continue;
    }
    obra += '%';
    obra += kCifras[octeto >> 4];
    obra += kCifras[octeto & 0x0F];
  }
  return obra;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
