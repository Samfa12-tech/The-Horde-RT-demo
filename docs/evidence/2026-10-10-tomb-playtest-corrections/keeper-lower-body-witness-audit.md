# Keeper lower-body witness audit

- Source: `assets/models/enemies/meshy/lich_placeholder_merged_animations_v01.glb`
- SHA-256: `049979a83aca55358f54af8d3af1f7d518607bef634474a4ef015bfdff947a42`
- Pose used for neutral pins: `Idle_02` at `timeSeconds = 0.0` (first authored key, native skin transform path reproduced from glTF TRS, parent hierarchy, inverse-bind matrices, and four-weight JOINTS_0/WEIGHTS_0 blend).
- Output mapping: expanded SkinTextured output index `n` reads original primitive index accessor element `n`; each selected entry below is verified to have all nonzero weights above `1e-4` on one or more of the nine lower-body joint names only. 12 distinct expanded outputs are selected for each joint (108 unique total).
- Neutral staff centroid reproduced from the 40 audited emissive outputs: `(0.972983, 0.800802, 0.605776)`; native smoke log: `(0.972983, 0.800802, 0.605775)`.

Each witness is `expandedIndex: neutralLocalPositionXYZ`; positions are metres in the `SkinTextured` output frame.

## LeftUpLeg
- `1480`: `(0.0257590,0.4084128,-0.0403042)`; weights `RightLeg=0.5059,LeftLeg=0.4419,LeftUpLeg=0.0273,RightUpLeg=0.0249`
- `2379`: `(0.0728322,0.4409688,0.0078306)`; weights `LeftLeg=0.5956,RightLeg=0.2731,LeftUpLeg=0.1155,RightUpLeg=0.0159`
- `5147`: `(0.2743080,0.3452131,0.0258534)`; weights `LeftLeg=0.7553,LeftFoot=0.2107,LeftUpLeg=0.0340`
- `17394`: `(0.2113380,0.6682832,0.2530816)`; weights `LeftUpLeg=0.9930,LeftLeg=0.0070`
- `19197`: `(-0.2400115,0.5882741,0.1460424)`; weights `RightUpLeg=0.9519,LeftUpLeg=0.0481`
- `20657`: `(0.1205796,0.8110898,0.2723000)`; weights `LeftUpLeg=0.7344,Hips=0.2656`
- `21316`: `(0.0966921,0.7751605,0.2982225)`; weights `LeftUpLeg=0.8844,Hips=0.1156`
- `21934`: `(0.0596541,0.3063156,0.3543101)`; weights `LeftLeg=0.5682,RightLeg=0.2093,LeftFoot=0.1454,LeftUpLeg=0.0771`
- `22283`: `(-0.0124271,0.9166936,0.2807455)`; weights `Hips=0.7365,LeftUpLeg=0.2635`
- `22744`: `(0.0298529,0.5112085,0.3798616)`; weights `LeftUpLeg=0.6568,RightUpLeg=0.2191,LeftLeg=0.1077,RightLeg=0.0164`
- `23149`: `(-0.0132246,0.4110307,0.3587016)`; weights `LeftUpLeg=0.3394,LeftLeg=0.3061,RightUpLeg=0.1854,RightLeg=0.1690`
- `24584`: `(0.1721617,0.3184456,0.4137515)`; weights `LeftLeg=0.6315,LeftFoot=0.2170,LeftUpLeg=0.0996,RightLeg=0.0519`

## LeftLeg
- `0`: `(0.0643266,0.0862346,-0.4329597)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2524,LeftFoot=0.1422`
- `399`: `(0.2529689,0.0732632,-0.2950334)`; weights `LeftLeg=0.3260,LeftFoot=0.3260,RightLeg=0.2108,RightFoot=0.1372`
- `798`: `(0.2526039,0.0842695,-0.2388486)`; weights `LeftLeg=0.3422,LeftFoot=0.3422,RightLeg=0.1941,RightFoot=0.1216`
- `1286`: `(0.3026815,0.0570391,-0.1978162)`; weights `LeftLeg=0.4183,LeftFoot=0.4183,RightLeg=0.1087,RightFoot=0.0546`
- `1877`: `(0.3142273,0.1046785,-0.1352221)`; weights `LeftLeg=0.4641,LeftFoot=0.4641,RightLeg=0.0525,RightFoot=0.0193`
- `2562`: `(0.2829731,0.1792099,-0.0690795)`; weights `LeftLeg=0.4773,LeftFoot=0.4773,RightLeg=0.0369,RightFoot=0.0084`
- `6891`: `(0.3993072,0.1446941,0.0557295)`; weights `LeftFoot=0.6219,LeftLeg=0.3781`
- `19440`: `(-0.2648303,0.2956602,0.1156323)`; weights `RightLeg=0.7816,RightFoot=0.0826,RightUpLeg=0.0680,LeftLeg=0.0678`
- `21274`: `(-0.2460259,0.3491789,0.1921817)`; weights `RightLeg=0.6276,RightUpLeg=0.2183,LeftLeg=0.1104,LeftUpLeg=0.0437`
- `22435`: `(-0.0816289,0.2950031,0.2937419)`; weights `RightLeg=0.4287,LeftLeg=0.3810,LeftFoot=0.0996,RightUpLeg=0.0907`
- `23316`: `(-0.0275724,0.4183229,0.3584483)`; weights `LeftUpLeg=0.3480,LeftLeg=0.2720,RightUpLeg=0.2113,RightLeg=0.1687`
- `27311`: `(0.2644287,0.0838967,0.4566589)`; weights `LeftToeBase=0.9158,LeftFoot=0.0816,LeftLeg=0.0014,RightFoot=0.0011`

## LeftFoot
- `1`: `(0.0688494,0.0877178,-0.4350993)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2524,LeftFoot=0.1422`
- `519`: `(0.2771973,0.0694609,-0.2779024)`; weights `LeftLeg=0.3322,LeftFoot=0.3322,RightLeg=0.2045,RightFoot=0.1311`
- `1043`: `(0.0075339,0.1249572,-0.3441315)`; weights `RightLeg=0.3894,RightFoot=0.3894,LeftLeg=0.1568,LeftFoot=0.0643`
- `1764`: `(0.3820270,0.0505033,-0.1849462)`; weights `LeftLeg=0.4957,LeftFoot=0.4957,RightLeg=0.0083,RightFoot=0.0003`
- `6092`: `(0.1757281,0.0431994,-0.0267182)`; weights `LeftFoot=0.4264,LeftLeg=0.4131,RightLeg=0.1022,RightFoot=0.0582`
- `17979`: `(-0.3564361,0.1159132,0.0158241)`; weights `RightToeBase=0.8930,RightFoot=0.0822,RightLeg=0.0241,LeftFoot=0.0007`
- `19690`: `(-0.2647593,0.0609071,0.0921958)`; weights `RightToeBase=0.5919,RightFoot=0.3352,LeftFoot=0.0676,RightLeg=0.0054`
- `21202`: `(-0.0767027,0.0834256,0.2755008)`; weights `LeftFoot=0.4878,RightFoot=0.3210,RightToeBase=0.1635,LeftLeg=0.0277`
- `22339`: `(0.1010386,0.0619679,0.3625435)`; weights `LeftFoot=0.7456,LeftToeBase=0.1081,RightFoot=0.1044,RightToeBase=0.0419`
- `23268`: `(0.1573022,0.0925846,0.3838904)`; weights `LeftFoot=0.7247,LeftToeBase=0.1904,RightFoot=0.0525,LeftLeg=0.0324`
- `24520`: `(0.2272571,0.0377786,0.4207588)`; weights `LeftToeBase=0.6822,LeftFoot=0.3081,RightFoot=0.0066,RightToeBase=0.0031`
- `27454`: `(0.2179333,0.0595666,0.4809147)`; weights `LeftToeBase=0.8903,LeftFoot=0.1063,RightFoot=0.0020,RightToeBase=0.0014`

## LeftToeBase
- `4587`: `(0.5139811,0.0244287,0.0186102)`; weights `LeftFoot=0.6887,LeftLeg=0.3099,LeftToeBase=0.0014`
- `11171`: `(0.5712060,0.0191672,0.0921868)`; weights `LeftFoot=0.7812,LeftLeg=0.1949,LeftToeBase=0.0239`
- `21823`: `(0.1673863,0.0399841,0.3610679)`; weights `LeftFoot=0.7457,LeftToeBase=0.1954,RightFoot=0.0424,RightToeBase=0.0165`
- `22373`: `(0.1109391,0.1295416,0.3678515)`; weights `LeftFoot=0.7277,RightFoot=0.1036,LeftLeg=0.1027,LeftToeBase=0.0660`
- `23102`: `(0.2709424,0.1448343,0.3485592)`; weights `LeftFoot=0.7873,LeftToeBase=0.1141,LeftLeg=0.0967,RightFoot=0.0019`
- `23605`: `(0.2232561,0.0606667,0.3637178)`; weights `LeftFoot=0.7311,LeftToeBase=0.2525,RightFoot=0.0115,RightToeBase=0.0048`
- `24035`: `(0.2325240,0.0274206,0.3779331)`; weights `LeftFoot=0.5495,LeftToeBase=0.4399,RightFoot=0.0075,RightToeBase=0.0031`
- `24526`: `(0.2468126,0.0395449,0.3947084)`; weights `LeftToeBase=0.6389,LeftFoot=0.3553,RightFoot=0.0039,RightToeBase=0.0018`
- `25120`: `(0.2466293,0.0577012,0.4283898)`; weights `LeftToeBase=0.8053,LeftFoot=0.1897,RightFoot=0.0032,RightToeBase=0.0018`
- `25977`: `(0.2384440,0.0464727,0.4407685)`; weights `LeftToeBase=0.8183,LeftFoot=0.1768,RightFoot=0.0032,RightToeBase=0.0018`
- `27166`: `(0.2309469,0.0661903,0.4787755)`; weights `LeftToeBase=0.9196,LeftFoot=0.0779,RightFoot=0.0014,RightToeBase=0.0011`
- `27563`: `(0.3548311,0.0506101,0.6125354)`; weights `LeftToeBase=0.9997,RightLeg=0.0003`

## RightUpLeg
- `1608`: `(0.0153226,0.4155366,-0.0647080)`; weights `RightLeg=0.5660,LeftLeg=0.3835,RightUpLeg=0.0318,LeftUpLeg=0.0187`
- `2231`: `(0.0583970,0.4616192,-0.0121182)`; weights `LeftLeg=0.5244,RightLeg=0.3011,LeftUpLeg=0.1301,RightUpLeg=0.0444`
- `2529`: `(0.0668164,0.4801045,0.0022926)`; weights `LeftLeg=0.5222,RightLeg=0.2448,LeftUpLeg=0.1823,RightUpLeg=0.0507`
- `18548`: `(-0.3109212,0.3239139,0.0587512)`; weights `RightLeg=0.8204,RightFoot=0.0778,RightUpLeg=0.0594,RightToeBase=0.0424`
- `19446`: `(-0.2429009,0.3006283,0.1298038)`; weights `RightLeg=0.7564,LeftLeg=0.0927,RightUpLeg=0.0842,RightFoot=0.0667`
- `19996`: `(-0.1745792,0.2830697,0.2124893)`; weights `RightLeg=0.6306,LeftLeg=0.2131,RightUpLeg=0.0832,LeftFoot=0.0731`
- `21024`: `(-0.1745792,0.2830697,0.2124893)`; weights `RightLeg=0.6306,LeftLeg=0.2131,RightUpLeg=0.0832,LeftFoot=0.0731`
- `21586`: `(-0.1019523,0.8525728,0.2494950)`; weights `Hips=0.6425,RightUpLeg=0.2657,LeftUpLeg=0.0918`
- `22438`: `(-0.2228004,0.3168726,0.2290763)`; weights `RightLeg=0.6133,LeftLeg=0.1841,RightUpLeg=0.1615,LeftFoot=0.0412`
- `22749`: `(-0.1015257,0.6437352,0.3007087)`; weights `RightUpLeg=0.5332,LeftUpLeg=0.4668`
- `23154`: `(-0.0690964,0.3505705,0.3389985)`; weights `LeftLeg=0.3560,RightLeg=0.3228,LeftUpLeg=0.1628,RightUpLeg=0.1583`
- `24093`: `(0.0910365,0.5224616,0.4035471)`; weights `LeftUpLeg=0.7416,LeftLeg=0.1306,RightUpLeg=0.1233,RightLeg=0.0046`

## RightLeg
- `2`: `(0.0670549,0.0777079,-0.4288436)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2524,LeftFoot=0.1422`
- `417`: `(0.1051367,0.0940218,-0.3273248)`; weights `RightLeg=0.2880,LeftLeg=0.2685,RightFoot=0.2662,LeftFoot=0.1773`
- `834`: `(-0.1046422,0.0899105,-0.5053703)`; weights `RightFoot=0.5375,RightLeg=0.4579,RightToeBase=0.0046`
- `1250`: `(0.2926655,0.0755111,-0.1943810)`; weights `LeftLeg=0.3947,LeftFoot=0.3947,RightLeg=0.1363,RightFoot=0.0743`
- `1680`: `(0.3506463,0.0469408,-0.1648545)`; weights `LeftLeg=0.4809,LeftFoot=0.4809,RightLeg=0.0300,RightFoot=0.0083`
- `2232`: `(-0.0073822,0.4782855,-0.0449075)`; weights `RightLeg=0.4815,LeftLeg=0.2890,RightUpLeg=0.1709,LeftUpLeg=0.0586`
- `3335`: `(-0.3065807,0.0828504,-0.2665905)`; weights `RightFoot=0.6743,RightToeBase=0.1966,RightLeg=0.1291`
- `18249`: `(-0.2730550,0.0804100,0.0778745)`; weights `RightToeBase=0.6171,RightFoot=0.3151,LeftFoot=0.0549,RightLeg=0.0129`
- `19449`: `(-0.2904067,0.3102922,0.0741749)`; weights `RightLeg=0.8078,RightFoot=0.0811,RightUpLeg=0.0634,RightToeBase=0.0477`
- `20823`: `(-0.0420947,0.1735231,0.2860475)`; weights `LeftFoot=0.4164,RightLeg=0.1973,RightFoot=0.1970,LeftLeg=0.1893`
- `22675`: `(0.1811705,0.2356746,0.3675271)`; weights `LeftFoot=0.4933,LeftLeg=0.4592,RightLeg=0.0308,RightFoot=0.0167`
- `3`: `(0.0828589,0.0850356,-0.4114131)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2522,LeftFoot=0.1424`

## RightFoot
- `4`: `(0.0670549,0.0777079,-0.4288436)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2524,LeftFoot=0.1422`
- `517`: `(-0.0026774,0.0860841,-0.5375494)`; weights `RightLeg=0.4798,RightFoot=0.4798,LeftLeg=0.0405`
- `1034`: `(0.3011578,0.0622386,-0.2241305)`; weights `LeftLeg=0.3672,LeftFoot=0.3672,RightLeg=0.1673,RightFoot=0.0983`
- `1557`: `(0.3350870,0.0675757,-0.1836159)`; weights `LeftLeg=0.4491,LeftFoot=0.4491,RightLeg=0.0715,RightFoot=0.0303`
- `2831`: `(-0.0788782,0.4057735,-0.1551109)`; weights `RightLeg=0.8841,LeftLeg=0.0872,RightUpLeg=0.0263,RightFoot=0.0024`
- `13867`: `(-0.4570986,0.0222920,-0.1460102)`; weights `RightFoot=0.5247,RightToeBase=0.4753`
- `18241`: `(-0.2134734,0.0769403,0.1435199)`; weights `RightFoot=0.4143,RightToeBase=0.3905,LeftFoot=0.1741,RightLeg=0.0212`
- `19695`: `(-0.1999373,0.0732676,0.1670851)`; weights `RightFoot=0.4152,RightToeBase=0.3458,LeftFoot=0.2188,RightLeg=0.0202`
- `21182`: `(-0.1223831,0.0606427,0.2400029)`; weights `LeftFoot=0.3911,RightFoot=0.3736,RightToeBase=0.2166,RightLeg=0.0188`
- `22637`: `(0.2221717,0.1451074,0.3679988)`; weights `LeftFoot=0.7373,LeftToeBase=0.1344,LeftLeg=0.1137,RightFoot=0.0146`
- `24187`: `(0.2051515,0.0327391,0.4078625)`; weights `LeftToeBase=0.5051,LeftFoot=0.4747,RightFoot=0.0141,RightToeBase=0.0061`
- `5`: `(0.0688494,0.0877178,-0.4350993)`; weights `RightLeg=0.3027,RightFoot=0.3027,LeftLeg=0.2524,LeftFoot=0.1422`

## RightToeBase
- `831`: `(-0.1046422,0.0899105,-0.5053703)`; weights `RightFoot=0.5375,RightLeg=0.4579,RightToeBase=0.0046`
- `1901`: `(-0.2146974,0.0512816,-0.3458786)`; weights `RightFoot=0.6091,RightLeg=0.3236,RightToeBase=0.0673`
- `5449`: `(-0.4703428,0.0014272,-0.3645294)`; weights `RightFoot=0.6164,RightToeBase=0.3836`
- `12950`: `(-0.3443983,0.1160393,-0.0975579)`; weights `RightFoot=0.5886,RightToeBase=0.3656,RightLeg=0.0457`
- `17341`: `(-0.2111168,0.0570124,0.1085969)`; weights `RightFoot=0.4545,RightToeBase=0.3954,LeftFoot=0.1400,RightLeg=0.0101`
- `18510`: `(-0.3082971,0.1023238,0.0242331)`; weights `RightToeBase=0.7560,RightFoot=0.2079,RightLeg=0.0257,LeftFoot=0.0103`
- `19392`: `(-0.2589857,0.0977501,0.1069667)`; weights `RightToeBase=0.5165,RightFoot=0.3598,LeftFoot=0.0899,RightLeg=0.0337`
- `20177`: `(-0.1863283,0.1214223,0.1658462)`; weights `RightFoot=0.4008,RightToeBase=0.2863,LeftFoot=0.2310,RightLeg=0.0820`
- `21458`: `(-0.0152254,0.0793039,0.3262482)`; weights `LeftFoot=0.6429,RightFoot=0.2292,RightToeBase=0.1018,LeftToeBase=0.0261`
- `23061`: `(0.1605381,0.0448159,0.3737188)`; weights `LeftFoot=0.7129,LeftToeBase=0.2227,RightFoot=0.0460,RightToeBase=0.0184`
- `24360`: `(0.1919011,0.0484910,0.4111686)`; weights `LeftFoot=0.5042,LeftToeBase=0.4691,RightFoot=0.0187,RightToeBase=0.0081`
- `832`: `(-0.1149525,0.0802457,-0.5300449)`; weights `RightFoot=0.5370,RightLeg=0.4594,RightToeBase=0.0036`

## Hips
- `12780`: `(0.1506025,0.8161392,0.2151331)`; weights `LeftUpLeg=0.6912,Hips=0.3088`
- `18879`: `(0.1765673,0.6941971,0.2725648)`; weights `LeftUpLeg=0.9957,Hips=0.0043`
- `19553`: `(-0.1519729,0.8926832,0.1859988)`; weights `Hips=0.7667,RightUpLeg=0.2333`
- `20410`: `(0.1509840,0.7340125,0.2910078)`; weights `LeftUpLeg=0.9676,Hips=0.0324`
- `20891`: `(0.1061956,0.8350417,0.2846702)`; weights `LeftUpLeg=0.6534,Hips=0.3466`
- `21120`: `(-0.0521646,0.9396081,0.2507904)`; weights `Hips=0.9347,LeftUpLeg=0.0653`
- `21566`: `(-0.1032263,0.8100433,0.2685144)`; weights `RightUpLeg=0.4446,Hips=0.2967,LeftUpLeg=0.2587`
- `21954`: `(-0.1243969,0.8378257,0.2512121)`; weights `Hips=0.4855,RightUpLeg=0.4102,LeftUpLeg=0.1043`
- `22228`: `(0.1161321,0.7709539,0.3148108)`; weights `LeftUpLeg=0.9068,Hips=0.0932`
- `22515`: `(0.0847355,0.7895809,0.3290314)`; weights `LeftUpLeg=0.8562,Hips=0.1436,RightUpLeg=0.0002`
- `22791`: `(-0.0464254,0.8909031,0.2642751)`; weights `Hips=0.8164,LeftUpLeg=0.1687,RightUpLeg=0.0149`
- `23499`: `(0.0439476,0.7806140,0.3310420)`; weights `LeftUpLeg=0.8413,Hips=0.0965,RightUpLeg=0.0622`
