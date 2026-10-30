#include "tui/baixas.hpp"
#include "tui/tokens.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace mysong::tui {
namespace {
using nucleo::EstadoDaBaixa;
using nucleo::EtapaDaBaixa;
using nucleo::RegistroDaBaixa;
using namespace ftxui;
using Relogio = std::chrono::steady_clock;

Color tinta(std::string_view token) {
  const auto tom = tokens::rgb(token);
  return Color::RGB(tom.r, tom.g, tom.b);
}
Color tinta_da_baixa(EstadoDaBaixa estado) {
  return tinta(estado == EstadoDaBaixa::Falhou ? tokens::crit
      : estado == EstadoDaBaixa::Concluido ? tokens::ok
      : estado == EstadoDaBaixa::Parado || estado == EstadoDaBaixa::Parando
          ? tokens::warn : tokens::v400);
}
std::string nome_do_estado(const RegistroDaBaixa& r) {
  switch (r.estado) {
    case EstadoDaBaixa::Aguardando: return "NA FILA";
    case EstadoDaBaixa::Preparando: return "PREPARANDO";
    case EstadoDaBaixa::Baixando: return std::string(nucleo::nome_da_etapa(r.etapa));
    case EstadoDaBaixa::Indexando: return "ATUALIZANDO BIBLIOTECA";
    case EstadoDaBaixa::Concluido: return "PRONTA PARA TOCAR";
    case EstadoDaBaixa::Falhou: return "FALHOU";
    case EstadoDaBaixa::Parando: return "PARANDO";
    case EstadoDaBaixa::Parado: return "PARADO";
  }
  return {};
}
std::string tempo(double segundos) {
  if (!std::isfinite(segundos) || segundos < 0) return "--";
  const auto n = static_cast<long long>(std::min(segundos, 315360000.0));
  if (n < 60) return std::to_string(n) + "s";
  return std::to_string(n / 60) + "m " + std::to_string(n % 60) + "s";
}
std::string bytes(double valor) {
  constexpr std::array<const char*, 4> unidades{"B", "KiB", "MiB", "GiB"};
  std::size_t unidade = 0;
  while (valor >= 1024 && unidade + 1 < unidades.size()) { valor /= 1024; ++unidade; }
  std::ostringstream texto;
  texto << std::fixed << std::setprecision(unidade ? 1 : 0) << valor << ' ' << unidades[unidade];
  return texto.str();
}
std::string duracao(const RegistroDaBaixa& r, Relogio::time_point agora) {
  return tempo(std::chrono::duration<double>(r.fim.value_or(agora) - r.inicio).count());
}
std::string atividade(const RegistroDaBaixa& r, Relogio::time_point agora) {
  if (r.estado == EstadoDaBaixa::Concluido) return "+";
  if (r.estado == EstadoDaBaixa::Falhou) return "!";
  if (r.estado == EstadoDaBaixa::Parado) return "-";
  constexpr std::array<const char*, 4> sinais{"|", "/", "-", "\\"};
  const auto pulso = std::chrono::duration_cast<std::chrono::seconds>(agora.time_since_epoch()).count();
  return sinais[static_cast<std::size_t>(pulso) % sinais.size()];
}
int grupo(EtapaDaBaixa etapa) {
  switch (etapa) {
    case EtapaDaBaixa::Preparando:
    case EtapaDaBaixa::Identificando:
    case EtapaDaBaixa::Procurando:
    case EtapaDaBaixa::Consultando: return 0;
    case EtapaDaBaixa::Transferindo:
    case EtapaDaBaixa::Convertendo: return 1;
    case EtapaDaBaixa::Capa:
    case EtapaDaBaixa::Letra:
    case EtapaDaBaixa::Canvas: return 2;
    case EtapaDaBaixa::Etiquetas:
    case EtapaDaBaixa::Verificando: return 3;
    case EtapaDaBaixa::Biblioteca:
    case EtapaDaBaixa::Pronta: return 4;
  }
  return 0;
}
Element trilha(const RegistroDaBaixa& r) {
  constexpr std::array<const char*, 5> nomes{"FONTE", "ÁUDIO", "EXTRAS", "ARQUIVO", "ACERVO"};
  Elements passos;
  const int atual = grupo(r.etapa);
  for (int i = 0; i < 5; ++i) {
    const bool ativo = i == atual && r.estado != EstadoDaBaixa::Concluido;
    bool feito = false;
    for (const auto etapa : r.etapas_concluidas)
      if (grupo(etapa) == i && i != atual) feito = true;
    if (r.estado == EstadoDaBaixa::Concluido && i == 4) feito = true;
    auto passo = text(std::string(ativo ? ">" : feito ? "+" : "·") + nomes[i]) | flex;
    passo = passo | color(tinta(ativo ? tokens::glow_ice : feito ? tokens::ok : tokens::text_muted));
    if (ativo) passo = passo | bold | bgcolor(tinta(tokens::raised));
    passos.push_back(std::move(passo));
  }
  return hbox(std::move(passos));
}
Element progresso(const RegistroDaBaixa& r) {
  if (r.estado == EstadoDaBaixa::Concluido)
    return hbox({text(" "), gauge(1) | flex, text("  DISPONÍVEL ")}) | color(tinta(tokens::ok));
  if (!r.porcentagem)
    return text(" " + nome_do_estado(r) + " · sem porcentagem disponível") | flex |
        color(tinta_da_baixa(r.estado));
  const int valor = std::clamp(*r.porcentagem, 0, 100);
  return hbox({text(" "), gauge(valor / 100.0f) | flex,
               text(" " + std::to_string(valor) + "% do áudio ")}) | color(tinta_da_baixa(r.estado));
}
Element metricas(const RegistroDaBaixa& r) {
  std::string texto = r.recebidos ? bytes(*r.recebidos) : "tamanho --";
  if (r.total) texto += " / " + std::string(r.total_estimado ? "~" : "") + bytes(*r.total);
  texto += "   " + (r.velocidade ? bytes(*r.velocidade) + "/s" : "velocidade --");
  texto += "   ETA " + (r.restante ? tempo(*r.restante) : "--");
  return text(" " + texto) | flex | color(tinta(tokens::text_body));
}
Element controles(const RegistroDaBaixa& r, std::size_t pagina,
                  std::size_t total, CaixasDasBaixas& caixas) {
  caixas.id = r.id;
  caixas.recomecar = r.estado == EstadoDaBaixa::Parado || r.estado == EstadoDaBaixa::Falhou;
  const bool parar = !nucleo::baixa_terminada(r.estado) && r.estado != EstadoDaBaixa::Parando &&
      r.estado != EstadoDaBaixa::Indexando;
  Element acao = text(caixas.recomecar ? " [R] Recomeçar " : parar ? " [P] Parar "
      : r.estado == EstadoDaBaixa::Indexando ? " Confirmando... "
      : r.estado == EstadoDaBaixa::Parando ? " Parando... " : " Disponível ");
  if (parar || caixas.recomecar) acao = acao | bold | inverted | reflect(caixas.acao);
  else acao = acao | dim;
  return hbox({text(" < ") | reflect(caixas.anterior),
      text(std::to_string(pagina + 1) + "/" + std::to_string(total)),
      text(" V > ") | reflect(caixas.seguinte), filler(), std::move(acao)});
}
std::string resumo(const nucleo::Andamento& a) {
  std::size_t prontas = 0, indexando = 0, falhas = 0;
  for (const auto& r : a.registros) {
    prontas += r.estado == EstadoDaBaixa::Concluido;
    indexando += r.estado == EstadoDaBaixa::Indexando;
    falhas += r.estado == EstadoDaBaixa::Falhou;
  }
  return std::to_string(a.em_curso) + " ativas · " + std::to_string(a.na_espera) + " na fila · " +
      std::to_string(indexando) + " no índice · " + std::to_string(prontas) + " prontas · " +
      std::to_string(falhas) + " falhas";
}
}  // namespace

std::size_t AcompanhamentoDasBaixas::atualiza(
    const nucleo::Andamento& andamento, std::size_t pagina) {
  const auto& registros = andamento.registros;
  if (registros.empty()) {
    anterior_id_ = 0;
    anterior_terminou_ = true;
    return 0;
  }
  pagina %= registros.size();
  const auto novo = std::max_element(registros.begin(), registros.end(),
      [](const auto& a, const auto& b) { return a.id < b.id; });
  if (novo->id > maior_id_) {
    pagina = static_cast<std::size_t>(novo - registros.begin());
    maior_id_ = novo->id;
  } else if (registros[pagina].id == anterior_id_ && !anterior_terminou_ &&
             nucleo::baixa_terminada(registros[pagina].estado)) {
    const auto ativa = std::find_if(registros.begin(), registros.end(),
        [](const auto& r) { return !nucleo::baixa_terminada(r.estado); });
    if (ativa != registros.end()) pagina = static_cast<std::size_t>(ativa - registros.begin());
  }
  anterior_id_ = registros[pagina].id;
  anterior_terminou_ = nucleo::baixa_terminada(registros[pagina].estado);
  return pagina;
}

Element painel_das_baixas(const nucleo::Andamento& andamento, std::size_t pagina,
    std::size_t altura, CaixasDasBaixas& caixas, Relogio::time_point agora) {
  caixas = {};
  if (altura == 0) return emptyElement();
  Elements linhas;
  linhas.push_back(hbox({text(" ↓ TRANSFERÊNCIAS ") | bold | color(tinta(tokens::glow_ice)),
      filler(), text(" MYSONG / ACERVO ") | dim}));
  if (andamento.registros.empty()) {
    linhas.push_back(paragraph(" Busque com s ou cole um link com b."));
    linhas.push_back(text(" Cada etapa da música será acompanhada aqui.") | dim);
  } else {
    pagina %= andamento.registros.size();
    const auto& r = andamento.registros[pagina];
    const auto tom = tinta_da_baixa(r.estado);
    if (altura >= 13) linhas.push_back(text(" " + resumo(andamento)) | color(tinta(tokens::text_body)));
    linhas.push_back(text(" #" + std::to_string(r.id) + "  " + r.titulo) | bold | flex);
    if (altura >= 7) linhas.push_back(hbox({
        text(" " + atividade(r, agora) + " " + nome_do_estado(r)) | bold | color(tom) | flex,
        text(" " + duracao(r, agora) + " ") | dim}));
    if (altura >= 11) linhas.push_back(trilha(r));
    linhas.push_back(progresso(r));
    if (altura >= 11) linhas.push_back(metricas(r));
    if (altura >= 11) {
      const auto sem_noticia = std::chrono::duration_cast<std::chrono::seconds>(agora - r.ultima_noticia).count();
      std::string detalhe = r.estado == EstadoDaBaixa::Aguardando ? "Aguardando um espaço na fila de downloads"
          : r.detalhe.empty() ? "Preparando a próxima operação" : r.detalhe;
      if (!nucleo::baixa_terminada(r.estado) && r.estado != EstadoDaBaixa::Aguardando && sem_noticia >= 15)
        detalhe = "Sem nova informação há " + tempo(sem_noticia) + ". " + detalhe;
      linhas.push_back(paragraph(" " + detalhe) | size(HEIGHT, EQUAL, altura >= 13 ? 2 : 1));
      const std::string destino = r.caminho.empty() ? "Destino será confirmado após resolver os dados da faixa"
          : r.caminho.string();
      linhas.push_back(text(" " + destino) | flex | color(tinta(tokens::text_muted)));
    }
    if (altura >= 13) {
      const std::string aviso = r.avisos.empty() ? "Extras: falhas opcionais não impedem o áudio de ficar disponível"
          : "Avisos (" + std::to_string(r.avisos.size()) + "): " + r.avisos.back();
      linhas.push_back(paragraph(" " + aviso) | size(HEIGHT, EQUAL, 2) |
          color(tinta(r.avisos.empty() ? tokens::text_muted : tokens::warn)));
      linhas.push_back(text(" " + std::string(nucleo::nome_da_fonte(r.fonte)) + " · etapa há " +
          tempo(std::chrono::duration<double>(r.fim.value_or(agora) - r.inicio_da_etapa).count())) | dim);
      // Mostra as próximas músicas sem tirar o foco do pedido acompanhado.
      std::string outras = " Fila: ";
      for (const auto& outra : andamento.registros)
        if (outra.id != r.id && !nucleo::baixa_terminada(outra.estado))
          outras += "#" + std::to_string(outra.id) + " " + outra.titulo + " [" +
              nome_do_estado(outra) + (outra.porcentagem ? " " + std::to_string(*outra.porcentagem) + "%" : "") + "] · ";
      linhas.push_back(text(outras == " Fila: " ? " Nenhuma outra transferência pendente" : outras) | flex | dim);
    }
    linhas.push_back(controles(r, pagina, andamento.registros.size(), caixas));
  }
  Element painel = vbox(std::move(linhas)) | color(tinta(tokens::text_primary)) | bgcolor(tinta(tokens::panel));
  if (altura >= 7) painel = painel | borderRounded | color(tinta(tokens::line_bright));
  return painel | size(HEIGHT, EQUAL, static_cast<int>(altura));
}
}  // namespace mysong::tui
