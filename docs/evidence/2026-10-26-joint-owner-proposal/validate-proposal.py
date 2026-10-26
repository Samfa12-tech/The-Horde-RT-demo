"""Offline proposal arithmetic/negative cases only; never launches a game or changes ABI."""
from pathlib import Path
import copy
import json

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PLAN = ROOT / 'docs/design/data/horde-1.7-ring-flask-owner-proposal.json'


def validate(p, baseline):
    def require(condition, reason):
        if not condition:
            raise ValueError(reason)
    require(p['status'] == 'pending-owner-approval' and not p['runtimeEnabled'], 'not an admission')
    require(p['preservedMetadataIndices'] == list(range(28)), 'existing owners must remain')
    require(p['forestMasterMetadataIndices'] == [26, 27], 'master metadata must remain')
    roles = p['roles']; limits = p['requestedLimits']; materials = p['materialPacking']
    require([r['role'] for r in roles] == ['specialWarning', 'rightHandFlask'], 'two distinct roles')
    require([r['metadataIndex'] for r in roles] == [28, 29], 'metadata collision or changed assignment')
    require([r['ordinaryPhysicalSlot'] for r in roles] == [28, 29], 'physical owner collision')
    require([r['assetIndex'] for r in roles] == [13, 14], 'unique appended asset routes required')
    require([r['mutableMaterial'] for r in roles] == [True, False], 'warning owns mutable factors')
    require(all(r['primitiveCount'] == r['sourceMaterials'] == 1 for r in roles), 'bounded asset contracts')
    require(limits == dict(ordinaryDefinitions=30, instanceMetadata=30, staticAssets=15,
                          primitives=38, materials=34, tlasInstances=106,
                          ordinaryBlasMax=23, sharedBlasMax=25), 'unreviewed bounds')
    require(materials['baselineAfterExistingAliases'] == baseline['ordinary']['packedMaterialRows'] == 30,
            'measured packed material basis')
    require(materials['existingChestAliases'] == baseline['sourceFacts']['existingChestAliasRows'] == 2,
            'retain existing exact aliases')
    require(materials['baselinePreAlias'] == 32 and materials['showcaseWorldPalette'] == 2 and
            materials['newSourceMaterials'] == sum(r['sourceMaterials'] for r in roles) == 2 and
            materials['newFlaskAliasAssumed'] == 0, 'no unproved flask alias')
    require(materials['candidatePreAlias'] == materials['baselinePreAlias'] + materials['newSourceMaterials'] == 34,
            'pre-alias peak must fit intake')
    require(materials['candidateAfterExistingAliases'] == materials['candidatePreAlias'] - 2 == 32 and
            materials['candidateFinal'] == materials['candidateAfterExistingAliases'] + 2 == 34,
            'world palette must also fit')
    require(max(materials['candidatePreAlias'], materials['candidateFinal']) <= limits['materials'], 'material overflow')
    require([r['name'] for r in p['profiles']] == ['baked', 'shared64', 'shared76Diagnostic'], 'preserve profiles')
    for profile, trees in zip(p['profiles'], [0, 64, 76]):
        require(profile['trees'] == trees and profile['ordinaryBase'] == 30, 'forest offset must follow both roles')
        require(profile['physicalDefinitions'] == 30 + trees <= limits['tlasInstances'], 'TLAS overlap/truncation')
        expected_primitives = (baseline['ordinary']['primitiveRows'] if not trees else
                               baseline['shared76']['primitiveRows']) + 2
        require(profile['primitiveRows'] == expected_primitives <= limits['primitives'], 'primitive overflow')
        require(profile['uniqueAssets'] == baseline['shared76']['uniqueStaticAssets'] + 2 == 15,
                'asset count must cover both owners')
        require(profile['finalMaterials'] == materials['candidateFinal'], 'profile material count')
        require(profile['blasMaximum'] == (25 if trees else 23), 'BLAS ceiling must include both assets')
    require(p['concurrentSpecialWarnings'] == 1, 'not a multi-warning pool')
    require(p['newTextureLayers'] == p['newLights'] == p['newFireEmitters'] == 0, 'extra rendering scope')
    require(not p['recordLayoutsChanged'] and not p['descriptorBindingsChanged'] and p['framesInFlight'] == 1,
            'preserve transport/synchronization ABI')
    require(p['loadedOwnedBlasRequiredBeforeAnyVisibility'] and
            not p['normalFeatureEnablementChanged'] and p['ordinaryRetentionMustRevalidateNewQuerySources'],
            'asset/retention/default gates must remain')


def main():
    p = json.loads(PLAN.read_text()); baseline = json.loads((HERE/'baseline.json').read_text())
    abi = json.loads((ROOT/'src/vulkan/raytracing/RtSceneAbi.def').read_text())
    for name in ['instanceMetadata', 'staticAssets', 'primitives', 'materials', 'tlasInstances']:
        assert p['currentLimits'][name] == abi['capacities'][name]
    assert p['currentLimits']['ordinaryDefinitions'] == abi['sceneCounts']['ordinaryShowcaseTlasInstances']
    validate(p, baseline)
    mutations = [
        ('same metadata owner', lambda x: x['roles'][1].update(metadataIndex=28)),
        ('same physical slot', lambda x: x['roles'][1].update(ordinaryPhysicalSlot=28)),
        ('same static asset', lambda x: x['roles'][1].update(assetIndex=13)),
        ('borrow master provider', lambda x: x['roles'][0].update(metadataIndex=26)),
        ('truncate76 profile', lambda x: x['profiles'][2].update(physicalDefinitions=105)),
        ('old forest base', lambda x: x['profiles'][1].update(ordinaryBase=28)),
        ('assumed late flask alias', lambda x: x['materialPacking'].update(newFlaskAliasAssumed=1)),
        ('33-row intake', lambda x: x['requestedLimits'].update(materials=33)),
        ('old primitive bound', lambda x: x['requestedLimits'].update(primitives=36)),
        ('old TLAS bound', lambda x: x['requestedLimits'].update(tlasInstances=104)),
        ('second warning', lambda x: x.update(concurrentSpecialWarnings=2)),
        ('incomplete asset bypass', lambda x: x.update(loadedOwnedBlasRequiredBeforeAnyVisibility=False)),
        ('retention assumption', lambda x: x.update(ordinaryRetentionMustRevalidateNewQuerySources=False)),
        ('silent admission', lambda x: x.update(runtimeEnabled=True)),
    ]
    for name, mutate in mutations:
        changed = copy.deepcopy(p); mutate(changed)
        try:
            validate(changed, baseline)
        except ValueError:
            continue
        raise AssertionError('negative proposal accepted: ' + name)
    print(json.dumps({'scope': 'offline proposal arithmetic; not runtime admission',
                      'positiveProposalPassed': True, 'negativeCasesRejected': len(mutations),
                      'currentAbiUnchanged': True, 'ownerApprovalPending': True}, indent=2))


if __name__ == '__main__':
    main()
