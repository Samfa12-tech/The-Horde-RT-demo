import sys,subprocess
from pathlib import Path
root=Path.cwd()
checks=[('candidate-pipeline-freshness',['tools/compile-raygen.ps1','-CheckCatalog',
 '-ArtifactDirectory',str(root/'src/vulkan/raytracing/variants'),
 '-CatalogPath',str(root/'tools/raygen-variant-catalog.json'),
 '-BudgetPath',str(root/'tools/raygen-variant-budgets.json'),'-TestPublicationFaultAfter','9']),
 ('candidate-pipeline-adapter-freshness',['tools/GenerateRtPipelineVariantCatalog.ps1','-Check']),
 ('candidate-query-freshness',['tools/GenerateRayQueryComputeVariants.ps1','-Check']),
 ('candidate-compatibility-freshness',['tools/compile-raygen.ps1','-Check']),
 ('candidate-legacy-compatibility-freshness',['tools/compile-raygen.ps1','-Legacy','-Check'])]
for label,args in checks:
 result=subprocess.call([sys.executable,'reports/wp2a-combat/run.py',label,'pwsh','-NoProfile','-File',*args])
 if result: sys.exit(result)
