#include <algorithm>
#include <array>
#include <iostream>
#include <string>

#include "scene/assets/PlayerPrimitiveContract.h"
#include "scene/assets/PlayerViewmodelContract.h"

int main()
{
    using namespace horde::scene::assets;
    bool ok = true;
    const auto check = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << message << '\n'; ok = false; }
    };
    std::string diagnostic;
    std::array<PlayerPrimitiveDeclaration, 4> declarations{{
        {"BodyPrimaryVisible", true, true, true},
        {"HeadPrimaryMasked", false, true, true},
        {"NearFacePrimaryMasked", false, true, true},
        {"GauntletPrimaryVisible", true, true, true},
    }};
    check(ValidatePlayerPrimitiveDeclarations(declarations, diagnostic) && diagnostic.empty(),
          "The exact four-way declaration must be valid");
    std::array<unsigned, 4> order{{0, 1, 2, 3}};
    unsigned permutations = 0;
    do {
        std::array<std::string_view, 4> names{};
        std::array<PlayerPrimitiveDeclaration, 4> reordered{};
        for (unsigned i = 0; i < order.size(); ++i) {
            names[i] = declarations[order[i]].material;
            reordered[i] = declarations[order[i]];
        }
        check(ValidatePlayerPrimitiveNames(names, diagnostic), "GLB primitive order must not define semantics");
        check(ValidatePlayerPrimitiveDeclarations(reordered, diagnostic), "Manifest order must not define semantics");
        ++permutations;
    } while (std::next_permutation(order.begin(), order.end()));
    check(permutations == 24, "All four-way permutations must be exercised");
    check(!ValidatePlayerPrimitiveDeclarations(std::span(declarations).first(3), diagnostic) && !diagnostic.empty(),
          "Stale three-way manifests must be rejected");
    auto bad = declarations;
    bad[3].material = "BodyPrimaryVisible";
    check(!ValidatePlayerPrimitiveDeclarations(bad, diagnostic), "Duplicate semantics must be rejected");
    bad[3].material = "gauntletprimaryvisible";
    check(!ValidatePlayerPrimitiveDeclarations(bad, diagnostic), "Unknown or case-folded semantics must be rejected");
    for (unsigned index = 0; index < declarations.size(); ++index) {
        bad = declarations;
        bad[index].firstPersonPrimary = !bad[index].firstPersonPrimary;
        check(!ValidatePlayerPrimitiveDeclarations(bad, diagnostic), "Conflicting primary visibility must be rejected");
        bad = declarations;
        bad[index].shadow = false;
        check(!ValidatePlayerPrimitiveDeclarations(bad, diagnostic), "Conflicting shadow visibility must be rejected");
        bad = declarations;
        bad[index].reflection = false;
        check(!ValidatePlayerPrimitiveDeclarations(bad, diagnostic), "Conflicting reflection visibility must be rejected");
    }
    std::array<std::string_view, 5> extra{{"BodyPrimaryVisible", "HeadPrimaryMasked", "NearFacePrimaryMasked",
                                         "GauntletPrimaryVisible", "Unexpected"}};
    check(!ValidatePlayerPrimitiveNames(extra, diagnostic), "Extra primitives must not be silently accepted");
    extra[3] = "Unknown";
    check(!ValidatePlayerPrimitiveNames(std::span(extra).first(4), diagnostic), "Unknown GLB material names must fail closed");
    extra[3] = extra[0];
    check(!ValidatePlayerPrimitiveNames(std::span(extra).first(4), diagnostic), "Duplicate GLB semantics must fail closed");
    const auto* gauntlet = FindPlayerPrimitiveContract("GauntletPrimaryVisible");
    check(gauntlet && gauntlet->semantic == PlayerPrimitiveSemantic::GauntletPrimaryVisible &&
          gauntlet->firstPersonPrimary && gauntlet->textureGroup == PlayerTextureGroup::Gauntlet,
          "Gauntlet must be explicitly primary-visible with its own texture group");
    for (const auto name : {"BodyPrimaryVisible", "HeadPrimaryMasked", "NearFacePrimaryMasked"}) {
        const auto* part = FindPlayerPrimitiveContract(name);
        check(part && part->textureGroup == PlayerTextureGroup::Body, "Body/head/near-face must share a named texture group");
    }
    check(!FindPlayerPrimitiveContract(static_cast<PlayerPrimitiveSemantic>(255)), "Invalid enum must not imply a body");
    check(!FindPlayerPrimitiveContract("Gauntlet"), "Partial names must not imply a semantic");
    check(ValidatePlayerPrimitiveDeclarations(declarations, diagnostic) && diagnostic.empty(),
          "A successful validation must clear a previous diagnostic");

    std::array<PlayerViewmodelPrimitiveDeclaration, 2> viewmodelDeclarations{{
        {"ViewmodelSleeves", true, false, false},
        {"ViewmodelGauntlets", true, false, false},
    }};
    check(ValidatePlayerViewmodelPrimitiveDeclarations(viewmodelDeclarations, diagnostic),
          "the exact two-part viewmodel contract must be valid");
    std::array<unsigned, 2> viewmodelOrder{{0, 1}};
    unsigned viewmodelPermutations = 0;
    do {
        std::array<std::string_view, 2> names{};
        std::array<PlayerViewmodelPrimitiveDeclaration, 2> reordered{};
        for (unsigned i = 0; i < viewmodelOrder.size(); ++i) {
            names[i] = viewmodelDeclarations[viewmodelOrder[i]].material;
            reordered[i] = viewmodelDeclarations[viewmodelOrder[i]];
        }
        check(ValidatePlayerViewmodelPrimitiveNames(names, diagnostic),
              "viewmodel primitive order must not define semantics");
        check(ValidatePlayerViewmodelPrimitiveDeclarations(reordered, diagnostic),
              "viewmodel declaration order must not define semantics");
        ++viewmodelPermutations;
    } while (std::next_permutation(viewmodelOrder.begin(), viewmodelOrder.end()));
    check(viewmodelPermutations == 2, "both viewmodel primitive orders must be exercised");
    check(!ValidatePlayerViewmodelPrimitiveDeclarations(
              std::span(viewmodelDeclarations).first(1), diagnostic),
          "missing viewmodel primitive must be rejected");
    auto badViewmodel = viewmodelDeclarations;
    badViewmodel[1].material = "BodyPrimaryVisible";
    check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
          "world-body contamination must be rejected from viewmodel declarations");
    badViewmodel[1].material = "ViewmodelSleeves";
    check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
          "duplicate viewmodel semantics must be rejected");
    badViewmodel = viewmodelDeclarations;
    badViewmodel[1].material = "UnknownViewmodelPart";
    check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
          "unknown viewmodel semantics must be rejected");
    for (unsigned index = 0; index < badViewmodel.size(); ++index) {
        badViewmodel = viewmodelDeclarations;
        badViewmodel[index].firstPersonPrimary = false;
        check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
              "viewmodel primary visibility conflicts must be rejected");
        badViewmodel = viewmodelDeclarations;
        badViewmodel[index].shadow = true;
        check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
              "viewmodel shadow visibility conflicts must be rejected");
        badViewmodel = viewmodelDeclarations;
        badViewmodel[index].reflection = true;
        check(!ValidatePlayerViewmodelPrimitiveDeclarations(badViewmodel, diagnostic),
              "viewmodel reflection visibility conflicts must be rejected");
    }
    const auto* sleeves = FindPlayerViewmodelPrimitiveContract("ViewmodelSleeves");
    const auto* gauntlets = FindPlayerViewmodelPrimitiveContract("ViewmodelGauntlets");
    check(sleeves != nullptr && sleeves->textureGroup == PlayerTextureGroup::Body &&
              gauntlets != nullptr && gauntlets->textureGroup == PlayerTextureGroup::Gauntlet,
          "viewmodel sleeves and gauntlets must retain named body/gauntlet texture groups");
    return ok ? 0 : 1;
}
