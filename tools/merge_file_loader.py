# Builds source/game_sa/FileLoader.cpp from source/game_sa/data/*.cpp
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
GAME_SA = ROOT / "source" / "game_sa"
DATA = GAME_SA / "data"
OUT = GAME_SA / "FileLoader.cpp"


def read(p: pathlib.Path) -> str:
    return p.read_text(encoding="utf-8")


def strip_first_include(text: str) -> str:
    return re.sub(r"^#include\s+[\"<][^\">]+[\">]\s*\n+", "", text.strip(), count=1)


def inner_of_d11_data(text: str) -> str:
    idx = text.find("namespace d11::data")
    if idx < 0:
        raise ValueError("namespace d11::data not found")
    text = text[idx:]
    m = re.match(r"^namespace d11::data\s*\{", text, flags=re.DOTALL)
    if not m:
        raise ValueError("Expected namespace d11::data wrapper")
    rest = text[m.end() :]
    depth = 1
    for j, ch in enumerate(rest):
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return rest[:j].strip("\n")
    raise ValueError("Unbalanced braces in namespace d11::data")


def split_anonymous_then_rest(inner: str) -> tuple[str, str]:
    inner = inner.lstrip()
    if not inner.startswith("namespace"):
        raise ValueError("Expected anonymous namespace first")
    i = inner.find("{")
    if i < 0:
        raise ValueError("No opening brace for anonymous namespace")
    depth = 0
    start = i
    for j in range(i, len(inner)):
        if inner[j] == "{":
            depth += 1
        elif inner[j] == "}":
            depth -= 1
            if depth == 0:
                anon_body = inner[start + 1 : j]
                rest = inner[j + 1 :].strip()
                return anon_body.strip("\n"), rest
    raise ValueError("Unbalanced braces in anonymous namespace")


def wrap_detail_named_anon(detail_name: str, anon_body: str, rest: str) -> str:
    parts = [
        f"namespace detail_{detail_name}",
        "{",
        "namespace",
        "{",
        anon_body,
        "}",
        rest,
        "}",
    ]
    return "\n".join(parts)


def strip_dat_cpp(text: str) -> str:
    return inner_of_d11_data(text)


def transform_img_block() -> str:
    raw = read(DATA / "ImgReader.cpp")
    inner = inner_of_d11_data(raw)
    anon_body, rest = split_anonymous_then_rest(inner)
    before_tolower, tolower_fn, after_tolower = extract_function_by_name(
        anon_body, "ToLowerAscii"
    )
    anon_body = (before_tolower + after_tolower).strip("\n")
    anon_body = anon_body.replace("ToLowerAscii(", "ImgToLowerAscii(")
    rest = rest.replace("ToLowerAscii(", "ImgToLowerAscii(")
    tolower_fn = tolower_fn.replace("ToLowerAscii", "ImgToLowerAscii")
    return "\n".join(
        [
            "namespace detail_img",
            "{",
            tolower_fn,
            "",
            "namespace",
            "{",
            anon_body,
            "}",
            "",
            rest.replace(
                "tImgParseResult ParseImgArchiveFile(const std::string& filePath)",
                "tImgParseResult ParseImgArchiveFileImpl(const std::string& filePath)",
                1,
            ),
            "}",
        ]
    )


def extract_function_by_name(body: str, name: str) -> tuple[str, str, str]:
    m = re.search(rf"([\s\S]*?)(std::string {name}\(std::string value\)\s*\{{)", body)
    if not m:
        raise ValueError(f"Function {name} not found")
    pre = m.group(1)
    start = m.start(2)
    i = body.find("{", start)
    depth = 0
    for j in range(i, len(body)):
        if body[j] == "{":
            depth += 1
        elif body[j] == "}":
            depth -= 1
            if depth == 0:
                func = body[start : j + 1]
                after = body[j + 1 :]
                return pre.rstrip(), func.strip(), after.lstrip()
    raise ValueError("Unbalanced function")


def transform_ide_ipl(reader_stem: str, short: str) -> str:
    path = DATA / f"{reader_stem}.cpp"
    inner = inner_of_d11_data(read(path))
    anon_body, rest = split_anonymous_then_rest(inner)
    rest = rest.replace(
        f"t{short}ParseResult Parse{short}File(const std::string& filePath)",
        f"t{short}ParseResult Parse{short}FileImpl(const std::string& filePath)",
        1,
    )
    return wrap_detail_named_anon(short.lower(), anon_body, rest)


def transform_dff(text: str) -> str:
    text = text.replace("#include \"DataTypes.h\"", '#include "GameDataFormat.h"', 1)
    inner = inner_of_d11_data(text)
    inner = inner.replace(
        "tDffParseResult ParseDffBuffer(const std::vector<std::uint8_t>& data)",
        "tDffParseResult ParseDffBufferImpl(const std::vector<std::uint8_t>& data)",
        1,
    )
    inner = inner.replace("return ParseDffBuffer(data);", "return ParseDffBufferImpl(data);")
    inner = inner.replace(
        "tDffParseResult ParseDffFile(const std::string& filePath)",
        "tDffParseResult ParseDffFileImpl(const std::string& filePath)",
        1,
    )
    inner = inner.replace(
        "tDffParseResult ParseDffFromtImgArchiveEntry(",
        "tDffParseResult ParseDffFromImgArchiveEntryImpl(",
        1,
    )
    return "namespace detail_dff\n{\n" + inner + "\n}\n"


def main() -> None:
    gta_inner = inner_of_d11_data(read(DATA / "GtaDatLoader.cpp"))
    gta_anon, gta_rest = split_anonymous_then_rest(gta_inner)
    gta_rest = gta_rest.replace(
        "void LoadGtaDatCatalog(", "void LoadGtaDatCatalogImpl(", 1
    )
    gta_block = wrap_detail_named_anon("gta_dat", gta_anon, gta_rest)

    ide_block = transform_ide_ipl("IdeReader", "Ide")
    ipl_block = transform_ide_ipl("IplReader", "Ipl")

    img_block = transform_img_block()

    dff_block = transform_dff(read(DATA / "DffReader.cpp"))

    tc_raw = read(DATA / "Timecyc.cpp").replace(
        "#include \"Timecyc.h\"", '#include "GameDataFormat.h"', 1
    )
    tc_inner = inner_of_d11_data(tc_raw)
    tc_anon, tc_rest = split_anonymous_then_rest(tc_inner)
    tc_rest = tc_rest.replace(
        "tTimecycFile ParseTimecycDat(const std::wstring& pathUtf16)",
        "tTimecycFile ParseTimecycDatImpl(const std::wstring& pathUtf16)",
        1,
    )
    tc_rest = tc_rest.replace(
        "void SampleSkyGradient(",
        "void SampleSkyGradientImpl(",
        1,
    )
    tc_block = wrap_detail_named_anon("timecyc", tc_anon, tc_rest)

    dat = strip_dat_cpp(read(DATA / "Dat.cpp"))
    ide_m = strip_dat_cpp(read(DATA / "Ide.cpp"))
    ipl_m = strip_dat_cpp(read(DATA / "Ipl.cpp"))
    zon = strip_dat_cpp(read(DATA / "Zon.cpp"))

    lines = [
        '#include "GameDataFormat.h"',
        "",
        "#include <algorithm>",
        "#include <cmath>",
        "#include <cctype>",
        "#include <cstdint>",
        "#include <cstring>",
        "#include <fstream>",
        "#include <locale>",
        "#include <numeric>",
        "#include <optional>",
        "#include <sstream>",
        "#include <string>",
        "#include <string_view>",
        "#include <utility>",
        "#include <vector>",
        "",
        "namespace d11::data",
        "{",
        gta_block,
        "",
        "void LoadGtaDatCatalog(const std::filesystem::path& gtaDatPath, tGtaDatParseSummary& outSummary)",
        "{",
        "    detail_gta_dat::LoadGtaDatCatalogImpl(gtaDatPath, outSummary);",
        "}",
        "",
        dat,
        "",
        ide_m,
        "",
        ipl_m,
        "",
        zon,
        "",
        ide_block,
        "",
        "tIdeParseResult ParseIdeFile(const std::string& filePath)",
        "{",
        "    return detail_ide::ParseIdeFileImpl(filePath);",
        "}",
        "",
        ipl_block,
        "",
        "tIplParseResult ParseIplFile(const std::string& filePath)",
        "{",
        "    return detail_ipl::ParseIplFileImpl(filePath);",
        "}",
        "",
        img_block,
        "",
        "tImgParseResult ParseImgArchiveFile(const std::string& filePath)",
        "{",
        "    return detail_img::ParseImgArchiveFileImpl(filePath);",
        "}",
        "",
        dff_block,
        "",
        "tDffParseResult ParseDffBuffer(const std::vector<std::uint8_t>& data)",
        "{",
        "    return detail_dff::ParseDffBufferImpl(data);",
        "}",
        "",
        "tDffParseResult ParseDffFile(const std::string& filePath)",
        "{",
        "    return detail_dff::ParseDffFileImpl(filePath);",
        "}",
        "",
        "tDffParseResult ParseDffFromImgArchiveEntry(const std::string& archivePath, const tImgArchiveEntry& entry)",
        "{",
        "    return detail_dff::ParseDffFromImgArchiveEntryImpl(archivePath, entry);",
        "}",
        "",
        tc_block,
        "",
        "tTimecycFile ParseTimecycDat(const std::wstring& pathUtf16)",
        "{",
        "    return detail_timecyc::ParseTimecycDatImpl(pathUtf16);",
        "}",
        "",
        "void SampleSkyGradient(const tTimecycFile& file, std::size_t weatherIndex, float minutesSinceMidnight, float outTopRgb[3], float outBottomRgb[3])",
        "{",
        "    detail_timecyc::SampleSkyGradientImpl(file, weatherIndex, minutesSinceMidnight, outTopRgb, outBottomRgb);",
        "}",
        "",
        "}",
        "",
    ]

    OUT.write_text("\n".join(lines), encoding="utf-8")
    print("Wrote", OUT)


if __name__ == "__main__":
    main()
