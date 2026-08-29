// O ESPIÃO do MusicBrainz (issue #57): URL de track do Spotify, ou artista,
// titulo e segundos, e sahe cada passo da resolução pelos MESMOS leitores da
// baixa: consulta, MBID, ISRCs e ficha. Custa até duas requisições, 1/segundo.
#include <cstdio>
#include <cstdlib>
#include <string>
#include "nucleo/musicbrainz.hpp"
namespace nu = mysong::nucleo;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "uso: espia_musicbrainz <url|artista titulo [seg]>\n");
    return 2;
  }
  std::string consulta, corpo;
  int ms = 0;
  const std::string alvo = argv[1];
  const std::size_t marca = alvo.find("track/");
  if (marca != std::string::npos) {
    std::string id = alvo.substr(marca + 6);
    const std::size_t fim = id.find_first_of("?&#/");
    if (fim != std::string::npos) id.resize(fim);
    consulta = nu::url_da_consulta_pelo_link(id);
  } else if (argc >= 3) {
    if (argc >= 4) {
      const int seg = std::atoi(argv[3]);  // cingido a um dia, como na baixa
      ms = seg > 0 && seg <= 86400 ? seg * 1000 : 0;
    }
    consulta = nu::url_da_consulta_pela_busca(alvo, argv[2], ms);
  }
  if (consulta.empty()) { std::fprintf(stderr, "alvo sem id nem titulo\n"); return 2; }
  std::printf("consulta: %s\n", consulta.c_str());
  if (nu::consulta_mb(consulta, &corpo) != nu::DesfechoMB::Achado) {
    std::puts("não achou");
    return 1;
  }
  const std::string mbid = marca != std::string::npos
                               ? nu::le_gravacao_da_url(corpo)
                               : nu::le_eleita_da_busca(corpo, ms);
  std::printf("gravação: %s\n", mbid.c_str());
  if (mbid.empty()) return 1;
  std::printf("ficha:    %s\n", nu::url_da_ficha(mbid).c_str());
  corpo.clear();
  if (nu::consulta_mb(nu::url_da_ficha(mbid), &corpo) != nu::DesfechoMB::Achado)
    return 1;
  const nu::FichaMB ficha = nu::le_ficha_da_gravacao(corpo);
  for (const std::string& isrc : ficha.isrcs) std::printf("isrc:     %s\n", isrc.c_str());
  std::printf("%s | %s | %d ms\nalbum: %s (%d) faixa %d\n", ficha.artista.c_str(),
              ficha.titulo.c_str(), ficha.duracao_ms, ficha.album.c_str(),
              ficha.ano, ficha.numero);
  return 0;
}
