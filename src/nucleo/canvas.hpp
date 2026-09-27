#pragma once
#include "nucleo/aquisicao.hpp"
#include <string_view>

namespace mysong::nucleo {
// § O Canvas é um video independente; sua falta jámais invalida o audio.
enum class ColheitaCanvas { Gravado, JaExiste, Indisponivel, Falhou };
// Juizos puros: entrada hostil dá cadeia vazia, sem tocar a rede ou o disco.
std::string id_da_faixa_spotify(std::string_view entrada);
std::string pedido_do_canvas(std::string_view id);
std::string url_do_canvas(std::string_view resposta, std::string_view id);
std::filesystem::path destino_do_canvas(const std::filesystem::path& raiz,
                                        const Pedido& pedido);
// Resolve a pagina publica em metadados; falso conserva o pedido original.
bool resolve_faixa_spotify(Pedido* pedido);
// Consulta limitada; video publicado inteiro e sem substituir arquivo existente.
ColheitaCanvas baixa_canvas(const std::filesystem::path& raiz, const Pedido& pedido);
}  // namespace mysong::nucleo
