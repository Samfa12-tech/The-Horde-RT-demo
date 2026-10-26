// Debug capture-only inspection of the pinned complete CPU source. No GPU mutation.
bool StaticWorldSpanSha256(std::span<const std::byte> bytes,std::string& result,std::string& error)
{
    if(bytes.size()>128u*1024u*1024u) {error="Static inspection hash exceeds128MiB.";return false;}
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectBytes=0,digestBytes=0,returned=0;
    auto status=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0);
    if(status>=0)status=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectBytes),sizeof(objectBytes),&returned,0);
    if(status>=0)status=BCryptGetProperty(algorithm,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&digestBytes),sizeof(digestBytes),&returned,0);
    std::vector<std::uint8_t> object(objectBytes),digest(digestBytes);
    if(status>=0)status=BCryptCreateHash(algorithm,&hash,object.data(),objectBytes,nullptr,0,0);
    if(status>=0 && !bytes.empty())status=BCryptHashData(hash,
        const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(bytes.data())),static_cast<ULONG>(bytes.size()),0);
    if(status>=0)status=BCryptFinishHash(hash,digest.data(),digestBytes,0);
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    if(status<0 || digestBytes!=32u) {error="Static inspection span SHA-256 failed.";return false;}
    std::ostringstream hex;hex<<std::hex<<std::setfill('0');
    for(const auto byte:digest)hex<<std::setw(2)<<static_cast<unsigned>(byte);
    result=hex.str();return true;
}

bool WriteStaticWorldInspectionFile(const std::filesystem::path& file,
    std::span<const std::byte> bytes,std::string& error)
{
    if(bytes.size()>128u*1024u*1024u) {error="Static inspection output exceeds128MiB.";return false;}
    const HANDLE handle=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE) {error="Static inspection requires fresh output: "+file.string();return false;}
    DWORD written=0;
    const bool success=WriteFile(handle,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)!=FALSE && written==bytes.size();
    const bool closed=CloseHandle(handle)!=FALSE;
    if(!success || !closed) {error="Static inspection write incomplete: "+file.string();return false;}
    return true;
}

// Canonical export fingerprints contain named payload hashes and explicit
// little-endian lengths, never pointer values, std::string storage or padding
// from an inspection-only record. The underlying spans are native GPU records.
void StaticWorldAppendU64(std::vector<std::byte>& bytes,std::uint64_t value)
{
    for(unsigned shift=0;shift<64u;shift+=8u)bytes.push_back(static_cast<std::byte>((value>>shift)&0xffu));
}
void StaticWorldAppendText(std::vector<std::byte>& bytes,std::string_view text)
{
    StaticWorldAppendU64(bytes,text.size());
    for(const unsigned char value:text)bytes.push_back(static_cast<std::byte>(value));
}
std::string StaticWorldHexU64(std::uint64_t value)
{
    std::ostringstream text;text<<std::hex<<std::setfill('0')<<std::setw(16)<<value;return text.str();
}

// A pinned measurement only, never an omission/admission certificate. A true
// structurallyValid reports the exact export fingerprint prerequisites; scene
// closure and current-frame eligibility remain separate engine-owned gates.
struct StaticWorldForestMeasurement {
    std::shared_ptr<const horde::vulkan::raytracing::RtStaticMeshSlot> source;
    std::uint64_t generation=0u;
    std::vector<std::uint32_t> ids;
    horde::scene::DetailBox bounds{};
    std::uint64_t count=0u;
    std::string inputFingerprint;
    bool structurallyValid=false;
};

bool InspectStaticWorldSourceImpl(const VulkanSurfaceContext& context,const std::filesystem::path& directory,
    const std::string& selectedMaterial,std::string& error,StaticWorldForestMeasurement* measurement)
{
    using namespace horde::vulkan::raytracing;
    using Vertex=horde::scene::assets::StaticRtVertex;
    using Region=horde::scene::assets::StaticTriangleRegion;
    static_assert(sizeof(Vertex)==64u && sizeof(Region)==1u && sizeof(std::uint32_t)==4u);
    const auto reject=[&](const char* reason){error=reason;return false;};
    const auto& launch=ParseCaptureLaunchOptions();
    const auto source=context.rtScene.DebugCachedCompleteStaticWorldSource();
    const auto inspectedGeneration=context.rtScene.StaticGeometryGeneration();
    const auto* asset=context.rtScene.DebugCollapseStaticAsset();
    if(!launch.captureHidden || launch.developmentCheckpoint.empty() || !context.useRtPath || !source || !asset ||
       selectedMaterial.empty() || selectedMaterial.size()>128u || !context.rtScene.StaticGeometryHasCompleteContributors())
        return reject("Static inspection requires the hidden ready chapter's complete cached source.");
    const bool writeOutput=measurement==nullptr;
    std::filesystem::path output;
    if(writeOutput) {
        std::error_code pathError;
#if defined(HORDE_RT_SOURCE_DIR)
        const auto reports=std::filesystem::weakly_canonical(std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports",pathError);
#else
        return reject("Static inspection requires a bound source checkout for ignored output.");
        const std::filesystem::path reports;
#endif
        if(pathError)return reject("Static inspection could not resolve the ignored reports root.");
        output=std::filesystem::weakly_canonical(directory,pathError);
        if(pathError)return reject("Static inspection could not resolve its output directory.");
        const auto relative=output.lexically_relative(reports);
        if(relative.empty() || relative.is_absolute() || output==reports || *relative.begin()=="..")
            return reject("Static inspection output must be a child of this workspace's reports directory.");
    }
    constexpr auto ownerIndex=PresentableTinyRtScene::kCollapseInstanceIndex;
    const auto& owner=source->InstanceMetadata()[ownerIndex];
    const auto& primitives=source->PrimitiveMetadata();const auto& vertexCounts=source->PrimitiveVertexCounts();
    const auto& vertices=source->Vertices();const auto& indices=source->Indices();const auto& regions=source->TriangleRegions();
    if(!owner.primitiveCount || owner.primitiveCount>32u || owner.primitiveCount!=asset->primitives.size() ||
       owner.geometryRole!=static_cast<std::uint32_t>(RtGeometryRole::Static) ||
       !(owner.flags&static_cast<std::uint32_t>(RtInstanceFlag::StaticPbr)) ||
       owner.primitiveBase>primitives.size() || owner.primitiveCount>primitives.size()-owner.primitiveBase ||
       vertexCounts.size()!=primitives.size() || indices.size()%3u || regions.size()!=indices.size()/3u ||
       !asset->HasValidTriangleProvenance() || asset->vertices.empty() || asset->indices.empty() ||
       asset->vertices.size()>128u*1024u*1024u/sizeof(Vertex) || asset->indices.size()>32u*1024u*1024u/4u ||
       asset->primitives.front().vertexOffset!=0u || asset->primitives.front().indexOffset!=0u)
        return reject("Static inspection rejected invalid or oversized complete world ownership.");
    const auto vertexBase=primitives[owner.primitiveBase].vertexOffset,indexBase=primitives[owner.primitiveBase].indexOffset;
    struct Row {
        std::string material,vertexSha,indexSha,regionSha;
        std::uint32_t packedRow=0,vertexOffset=0,vertexCount=0,indexOffset=0,indexCount=0;
        std::uint32_t assetVertexOffset=0,assetIndexOffset=0,materialRow=0,assetMaterialRow=0;
        std::array<std::uint64_t,3> regionCounts{};
    };
    std::vector<Row> rows;std::size_t selected=0,selectedCount=0,consumed=0;
    for(std::uint32_t local=0;local<owner.primitiveCount;++local) {
        const auto packedRow=owner.primitiveBase+local;
        const auto& p=primitives[packedRow];const auto& a=asset->primitives[local];
        const auto end=local+1u<asset->primitives.size()?asset->primitives[local+1u].vertexOffset:asset->vertices.size();
        if(a.vertexOffset>=end || end>asset->vertices.size() || a.indexOffset!=consumed ||
           !a.indexCount || a.indexCount%3u || a.indexOffset%3u || consumed>asset->indices.size() ||
           a.indexCount>asset->indices.size()-consumed || a.materialIndex>=asset->materials.size() ||
           a.nodeTransformIndex>=asset->nodeTransforms.size() || p.indexOffset%3u ||
           p.vertexOffset!=std::uint64_t(vertexBase)+a.vertexOffset || p.indexOffset!=std::uint64_t(indexBase)+a.indexOffset ||
           p.indexCount!=a.indexCount || vertexCounts[packedRow]!=end-a.vertexOffset ||
           p.vertexOffset>vertices.size() || end-a.vertexOffset>vertices.size()-p.vertexOffset ||
           p.indexOffset>indices.size() || p.indexCount>indices.size()-p.indexOffset || p.materialIndex>=source->Materials().size())
            return reject("Static inspection rejected a stale or malformed primitive span.");
        Row row;row.material=asset->materials[a.materialIndex].name;
        row.packedRow=packedRow;row.vertexOffset=p.vertexOffset;row.vertexCount=vertexCounts[packedRow];
        row.indexOffset=p.indexOffset;row.indexCount=p.indexCount;row.assetVertexOffset=a.vertexOffset;
        row.assetIndexOffset=a.indexOffset;row.materialRow=p.materialIndex;row.assetMaterialRow=a.materialIndex;
        const auto vertexBytes=std::as_bytes(std::span<const Vertex>{vertices.data()+p.vertexOffset,row.vertexCount});
        const auto indexBytes=std::as_bytes(std::span<const std::uint32_t>{indices.data()+p.indexOffset,p.indexCount});
        const auto regionSpan=std::span<const Region>{regions.data()+p.indexOffset/3u,p.indexCount/3u};
        if(std::memcmp(vertexBytes.data(),asset->vertices.data()+a.vertexOffset,vertexBytes.size())!=0 ||
           std::memcmp(indexBytes.data(),asset->indices.data()+a.indexOffset,indexBytes.size())!=0)
            return reject("Static inspection cached geometry differs from its complete assembled world asset.");
        for(const auto index:std::span<const std::uint32_t>{indices.data()+p.indexOffset,p.indexCount})
            if(index>=row.vertexCount)return reject("Static inspection found an invalid primitive-local index.");
        for(std::size_t i=0;i<regionSpan.size();++i) {
            const auto value=static_cast<unsigned>(regionSpan[i]);const auto assetTriangle=a.indexOffset/3u+i;
            const auto expected=assetTriangle<asset->triangleRegions.size()?asset->triangleRegions[assetTriangle]:Region::Shared;
            if(value>2u || regionSpan[i]!=expected)return reject("Static inspection found stale/corrupt triangle provenance.");
            ++row.regionCounts[value];
        }
        if(!StaticWorldSpanSha256(vertexBytes,row.vertexSha,error) || !StaticWorldSpanSha256(indexBytes,row.indexSha,error) ||
           !StaticWorldSpanSha256(std::as_bytes(regionSpan),row.regionSha,error))return false;
        if(row.material==selectedMaterial){selected=rows.size();++selectedCount;}
        rows.push_back(std::move(row));consumed+=a.indexCount;
    }
    if(consumed!=asset->indices.size() || selectedCount!=1u)
        return reject("Static inspection requires one uniquely named material primitive and fully owned indices.");

    // This is measurement only. A later, independently reviewed descriptor may
    // pin these values; exporting the current source never blesses it for use.
    struct SourcePayloadHash { const char* name;std::size_t bytes;std::string sha;bool available; };
    std::vector<SourcePayloadHash> sourceHashes;
    bool sourceHashesAvailable=true;
    std::vector<std::byte> sourceFingerprint;
    StaticWorldAppendText(sourceFingerprint,"horde-cached-static-payloads-v1");
    const auto hashSource=[&](const char* name,std::span<const std::byte> bytes) {
        SourcePayloadHash row{name,bytes.size(),{},bytes.size()<=128u*1024u*1024u};
        if(row.available && !StaticWorldSpanSha256(bytes,row.sha,error))return false;
        sourceHashesAvailable&=row.available;
        StaticWorldAppendText(sourceFingerprint,name);StaticWorldAppendU64(sourceFingerprint,bytes.size());
        StaticWorldAppendText(sourceFingerprint,row.sha);sourceHashes.push_back(std::move(row));return true;
    };
    if(!hashSource("staticVertices",std::as_bytes(std::span{source->Vertices()})) ||
       !hashSource("worldBodyVertices",std::as_bytes(std::span{source->Vertices(RtGeometryRole::PlayerWorldBody)})) ||
       !hashSource("viewmodelVertices",std::as_bytes(std::span{source->Vertices(RtGeometryRole::PlayerViewmodel)})) ||
       !hashSource("indices",std::as_bytes(std::span{source->Indices()})) ||
       !hashSource("triangleRegions",std::as_bytes(std::span{source->TriangleRegions()})) ||
       !hashSource("instanceMetadata",std::as_bytes(std::span{source->InstanceMetadata()})) ||
       !hashSource("primitiveMetadata",std::as_bytes(std::span{source->PrimitiveMetadata()})) ||
       !hashSource("primitiveVertexCounts",std::as_bytes(std::span{source->PrimitiveVertexCounts()})) ||
       !hashSource("geometryTransforms",std::as_bytes(std::span{source->GeometryTransforms()})) ||
       !hashSource("materials",std::as_bytes(std::span{source->Materials()})))return false;
    std::string sourceSha,ownerSha;
    if(sourceHashesAvailable && !StaticWorldSpanSha256(sourceFingerprint,sourceSha,error))return false;
    std::vector<std::byte> ownerFingerprint;
    StaticWorldAppendText(ownerFingerprint,"horde-complete-static-owner-v1");
    for(const auto value:{ownerIndex,owner.stableObjectId,owner.primitiveBase,owner.primitiveCount,
                          owner.assetIndex,owner.flags,owner.geometryRole})StaticWorldAppendU64(ownerFingerprint,value);
    for(const auto& row:rows) {
        StaticWorldAppendText(ownerFingerprint,row.material);
        for(const auto value:{row.packedRow,row.vertexOffset,row.vertexCount,row.indexOffset,row.indexCount,
                              row.assetVertexOffset,row.assetIndexOffset,row.materialRow,row.assetMaterialRow})
            StaticWorldAppendU64(ownerFingerprint,value);
        StaticWorldAppendText(ownerFingerprint,row.vertexSha);StaticWorldAppendText(ownerFingerprint,row.indexSha);
        StaticWorldAppendText(ownerFingerprint,row.regionSha);
    }
    if(!StaticWorldSpanSha256(ownerFingerprint,ownerSha,error))return false;

    constexpr std::array<std::string_view,4> forestNames{{"ForestBark","ForestPine","ForestMoss","ForestAlder"}};
    std::array<unsigned,4> forestRowCounts{};
    std::vector<std::uint32_t> forestIds;
    std::array<double,3> forestMinimum{{INFINITY,INFINITY,INFINITY}},forestMaximum{{-INFINITY,-INFINITY,-INFINITY}};
    bool forestFinite=true,forestExterior=true,forestTransformsIdentity=true,forestWouldEmptyPrimitive=false;
    constexpr std::array<float,12> identity{{1,0,0,0,0,1,0,0,0,0,1,0}};
    for(const auto& row:rows) {
        const auto family=std::find(forestNames.begin(),forestNames.end(),row.material);
        if(family==forestNames.end())continue;
        ++forestRowCounts[static_cast<std::size_t>(family-forestNames.begin())];
        forestTransformsIdentity &= row.packedRow<source->GeometryTransforms().size() &&
            source->GeometryTransforms()[row.packedRow]==identity;
        std::size_t removed=0;
        for(std::size_t local=0;local<row.indexCount;local+=3u) {
            bool selectedTriangle=true;
            for(unsigned corner=0;corner<3u;++corner) {
                const auto& point=vertices[row.vertexOffset+indices[row.indexOffset+local+corner]].position;
                for(unsigned axis=0;axis<3u;++axis)forestFinite &= std::isfinite(point[axis]);
                selectedTriangle &= std::isfinite(point[2]) && point[2]>24.f;
            }
            if(!selectedTriangle)continue;
            const auto triangle=static_cast<std::uint32_t>((row.indexOffset+local)/3u);
            forestIds.push_back(triangle);++removed;
            forestExterior &= regions[triangle]==Region::ExteriorDressing;
            for(unsigned corner=0;corner<3u;++corner) {
                const auto& point=vertices[row.vertexOffset+indices[row.indexOffset+local+corner]].position;
                for(unsigned axis=0;axis<3u;++axis) {
                    forestMinimum[axis]=std::min(forestMinimum[axis],double(point[axis]));
                    forestMaximum[axis]=std::max(forestMaximum[axis],double(point[axis]));
                }
            }
        }
        forestWouldEmptyPrimitive |= removed==row.indexCount/3u;
    }
    const bool forestFamiliesComplete=std::all_of(forestRowCounts.begin(),forestRowCounts.end(),[](unsigned n){return n==1u;});
    const bool forestStrictlyOrdered=std::adjacent_find(forestIds.begin(),forestIds.end(),
        [](std::uint32_t a,std::uint32_t b){return a>=b;})==forestIds.end();
    const bool forestBoundsAvailable=forestFamiliesComplete && forestFinite && !forestIds.empty() && forestTransformsIdentity;
    bool receiverEnvelope=false,targetBehindFaces=false;
    if(forestBoundsAvailable) {
        const horde::scene::DetailBox box{forestMinimum,forestMaximum};
        targetBehindFaces=horde::scene::DetailTargetBehindCoveredFaces(box);
        receiverEnvelope=horde::scene::DetailForestReceiverEnvelopeBlocked(box);
    }
    PresentableTinyRtScene::ForestDetailClosureObservation actualClosure{};
    if(forestBoundsAvailable)
        actualClosure=context.rtScene.DebugForestDetailClosure({forestMinimum,forestMaximum});
    const bool actualClosureAvailable=forestBoundsAvailable && actualClosure.ready &&
        actualClosure.sourceGeneration==inspectedGeneration;
    std::vector<std::byte> forestIdBytes;
    if(forestFamiliesComplete)for(const auto id:forestIds)
        for(unsigned shift=0;shift<32u;shift+=8u)forestIdBytes.push_back(static_cast<std::byte>((id>>shift)&0xffu));
    std::string forestIdsSha,forestExportSha;
    constexpr const char* forestIdFilename="static-world-ordinary-forest.triangles.u32le.bin";
    if(forestFamiliesComplete) {
        if(!StaticWorldSpanSha256(forestIdBytes,forestIdsSha,error))return false;
        if(writeOutput) {
            if(!WriteStaticWorldInspectionFile(output/forestIdFilename,forestIdBytes,error))return false;
            std::string writtenSha;
            if(!Sha256File(output/forestIdFilename,writtenSha,error))return false;
            if(writtenSha!=forestIdsSha)return reject("Forest selection export differs from its measured ID bytes.");
        }
    }
    std::array<std::uint64_t,6> forestBoundsBits{};
    if(forestBoundsAvailable)for(unsigned axis=0;axis<3u;++axis) {
        static_assert(sizeof(double)==sizeof(std::uint64_t));
        std::memcpy(&forestBoundsBits[axis],&forestMinimum[axis],sizeof(double));
        std::memcpy(&forestBoundsBits[axis+3u],&forestMaximum[axis],sizeof(double));
    }
    if(sourceHashesAvailable && forestBoundsAvailable && forestStrictlyOrdered && forestExterior && !forestWouldEmptyPrimitive) {
        std::vector<std::byte> measured;
        StaticWorldAppendText(measured,"horde-ordinary-forest-export-v1");
        StaticWorldAppendText(measured,sourceSha);StaticWorldAppendText(measured,ownerSha);
        StaticWorldAppendText(measured,forestIdsSha);StaticWorldAppendU64(measured,forestIds.size());
        StaticWorldAppendU64(measured,0x41c00000u); // Exact binary32 +24 source-Z plane.
        for(const auto bits:forestBoundsBits)StaticWorldAppendU64(measured,bits);
        if(!StaticWorldSpanSha256(measured,forestExportSha,error))return false;
    }
    if(measurement) {
        if(context.rtScene.StaticGeometryGeneration()!=inspectedGeneration ||
           context.rtScene.DebugCachedCompleteStaticWorldSource()!=source)
            return reject("Static inspection source identity changed during measurement.");
        measurement->source=source;
        measurement->generation=inspectedGeneration;
        measurement->count=forestIds.size();
        measurement->ids=std::move(forestIds);
        if(forestBoundsAvailable)measurement->bounds={forestMinimum,forestMaximum};
        measurement->inputFingerprint=std::move(forestExportSha);
        measurement->structurallyValid=!measurement->inputFingerprint.empty();
        error.clear();return true;
    }
    const auto& chosen=rows[selected];
    const auto vertexBytes=std::as_bytes(std::span<const Vertex>{vertices.data()+chosen.vertexOffset,chosen.vertexCount});
    const auto indexBytes=std::as_bytes(std::span<const std::uint32_t>{indices.data()+chosen.indexOffset,chosen.indexCount});
    const auto regionBytes=std::as_bytes(std::span<const Region>{regions.data()+chosen.indexOffset/3u,chosen.indexCount/3u});
    constexpr std::array<const char*,3> names{{"static-world-selected.vertices.bin","static-world-selected.indices.bin","static-world-selected.regions.bin"}};
    const std::array<std::span<const std::byte>,3> payloads{{vertexBytes,indexBytes,regionBytes}};
    std::array<std::string,3> fileHashes;
    for(std::size_t i=0;i<names.size();++i) {
        if(!WriteStaticWorldInspectionFile(output/names[i],payloads[i],error) || !Sha256File(output/names[i],fileHashes[i],error))return false;
    }
    if(fileHashes!=std::array<std::string,3>{{chosen.vertexSha,chosen.indexSha,chosen.regionSha}})
        return reject("Static inspection binary SHA-256 differs from the inspected immutable span.");
    std::ostringstream json;json.imbue(std::locale::classic());
    json<<"{\"schema\":1,\"scope\":\"Debug complete cached CPU world source; not GPU/visibility/LOD acceptance\","
        <<"\"assetRoot\":\""<<JsonEscape(ResolveAssetRoot().generic_string())<<"\","
        <<"\"checkpoint\":\""<<JsonEscape(launch.developmentCheckpoint)<<"\",\"staticGeometryGeneration\":"
        <<inspectedGeneration<<",\"completeContributors\":true,\"owner\":{\"instance\":"<<ownerIndex
        <<",\"stableObjectId\":"<<owner.stableObjectId<<",\"primitiveBase\":"<<owner.primitiveBase
        <<",\"primitiveCount\":"<<owner.primitiveCount<<",\"assetIndex\":"<<owner.assetIndex
        <<",\"flags\":"<<owner.flags<<",\"geometryRole\":"<<owner.geometryRole<<"},"
        <<"\"vertexLayout\":{\"strideBytes\":64,\"position\":0,\"normal\":16,\"tangent\":32,\"uv0\":48,"
        <<"\"fieldFormat\":\"float32x4\",\"byteOrder\":\"little-endian\"},\"indexStrideBytes\":4,\"regionStrideBytes\":1,"
        <<"\"regionValues\":{\"Shared\":0,\"TombDressing\":1,\"ExteriorDressing\":2},\"primitives\":[";
    for(std::size_t i=0;i<rows.size();++i) {
        const auto& row=rows[i];
        json<<(i?",":"")<<"{\"ownerLocalRow\":"<<i<<",\"cachedPrimitiveRow\":"<<row.packedRow
            <<",\"materialName\":\""<<JsonEscape(row.material)<<"\",\"cachedMaterialRow\":"<<row.materialRow
            <<",\"assetMaterialRow\":"<<row.assetMaterialRow<<",\"vertexOffset\":"<<row.vertexOffset
            <<",\"vertexCount\":"<<row.vertexCount<<",\"indexOffset\":"<<row.indexOffset
            <<",\"indexCount\":"<<row.indexCount<<",\"triangleOffset\":"<<row.indexOffset/3u
            <<",\"triangleCount\":"<<row.indexCount/3u<<",\"assetVertexOffset\":"<<row.assetVertexOffset
            <<",\"assetIndexOffset\":"<<row.assetIndexOffset<<",\"regionCounts\":["
            <<row.regionCounts[0]<<','<<row.regionCounts[1]<<','<<row.regionCounts[2]<<"],\"verticesSha256\":\""
            <<row.vertexSha<<"\",\"indicesSha256\":\""<<row.indexSha<<"\",\"regionsSha256\":\""<<row.regionSha<<"\"}";
    }
    json<<"],\"selected\":{\"materialName\":\""<<JsonEscape(selectedMaterial)<<"\",\"ownerLocalRow\":"<<selected
        <<",\"indicesArePrimitiveLocal\":true,\"cachedTriangleOffset\":"<<chosen.indexOffset/3u<<",\"files\":[";
    for(std::size_t i=0;i<names.size();++i)json<<(i?",":"")<<"{\"name\":\""<<names[i]
        <<"\",\"bytes\":"<<payloads[i].size()<<",\"sha256\":\""<<fileHashes[i]<<"\"}";
    json<<"]},\"cachedSourceIdentity\":{\"schema\":1,\"scope\":\"exact cached CPU payload bytes; no pointer, texture-image or GPU identity claim\","
        <<"\"pointerIdentitySerialized\":false,\"textureImagePayloadsIncluded\":false,\"staticGeometryGeneration\":"
        <<inspectedGeneration<<",\"payloadHashesAvailable\":"<<(sourceHashesAvailable?"true":"false")
        <<",\"payloadSha256\":"<<(sourceHashesAvailable?"\""+sourceSha+"\"":"null")
        <<",\"completeOwnerSha256\":\""<<ownerSha<<"\",\"fingerprintEncoding\":\"ordered length-prefixed names/hash text and u64le byte lengths; native GPU ABI spans\",\"payloads\":[";
    for(std::size_t i=0;i<sourceHashes.size();++i) {
        const auto& row=sourceHashes[i];
        json<<(i?",":"")<<"{\"name\":\""<<row.name<<"\",\"bytes\":"<<row.bytes
            <<",\"status\":\""<<(row.available?"measured":"unavailable-over128MiB")<<"\",\"sha256\":"
            <<(row.available?"\""+row.sha+"\"":"null")<<'}';
    }
    json<<"]},\"ordinaryForestOmissionExport\":{\"schema\":1,\"scope\":\"measurement for independent review only; not a visibility certificate\","
        <<"\"admissionGranted\":false,\"productionGuardChanged\":false,\"selectionRule\":\"all three source vertices strictly sourceZ>24 in four named forest families; world-space only with identity geometry transforms\","
        <<"\"thresholdFloat32Hex\":\"41c00000\","
        <<"\"families\":[";
    for(std::size_t i=0;i<forestNames.size();++i)json<<(i?",":"")<<"{\"name\":\""<<forestNames[i]<<"\",\"matchingPrimitiveRows\":"<<forestRowCounts[i]<<'}';
    json<<"],\"familiesCompleteAndUnique\":"<<(forestFamiliesComplete?"true":"false")
        <<",\"measurementStatus\":\""<<(forestFamiliesComplete?"measured":"unavailable-family-roster")<<'"'
        <<",\"allForestVerticesFinite\":"<<(forestFamiliesComplete?(forestFinite?"true":"false"):"null")
        <<",\"forestGeometryTransformsIdentity\":"<<(forestFamiliesComplete?(forestTransformsIdentity?"true":"false"):"null")
        <<",\"selectedCount\":"<<(forestFamiliesComplete?std::to_string(forestIds.size()):"null")
        <<",\"strictlyIncreasingIds\":"<<(forestFamiliesComplete?(forestStrictlyOrdered?"true":"false"):"null")
        <<",\"selectedProvenanceExteriorOnly\":"<<(forestFamiliesComplete?(forestExterior?"true":"false"):"null")
        <<",\"wouldEmptyRequiredPrimitive\":"<<(forestFamiliesComplete?(forestWouldEmptyPrimitive?"true":"false"):"null")
        <<",\"selectedIdsFile\":"<<(forestFamiliesComplete?"\""+std::string(forestIdFilename)+"\"":"null")
        <<",\"selectedIdsBytes\":"<<forestIdBytes.size()<<",\"selectedIdsSha256\":"
        <<(forestFamiliesComplete?"\""+forestIdsSha+"\"":"null")
        <<",\"bounds\":";
    if(forestBoundsAvailable) {
        json<<std::setprecision(17)<<"{\"minimum\":["<<forestMinimum[0]<<','<<forestMinimum[1]<<','<<forestMinimum[2]
            <<"],\"maximum\":["<<forestMaximum[0]<<','<<forestMaximum[1]<<','<<forestMaximum[2]<<"],\"float64HexMinThenMax\":[";
        for(unsigned i=0;i<forestBoundsBits.size();++i)json<<(i?",":"")<<'"'<<StaticWorldHexU64(forestBoundsBits[i])<<'"';
        json<<"]}";
    }else json<<"null";
    json<<",\"measuredInputFingerprintSha256\":"<<(forestExportSha.empty()?"null":"\""+forestExportSha+"\"")
        <<",\"closurePredicates\":{\"legacyCount\":{\"status\":\""<<(forestFamiliesComplete?"measured":"unavailable-family-roster")
        <<"\",\"expected\":84219,\"matches\":"<<(forestFamiliesComplete?(forestIds.size()==84219?"true":"false"):"null")<<"},"
        <<"\"targetBehindCoveredFaces\":{\"status\":\""<<(forestBoundsAvailable?"evaluated":"unavailable-invalid-selection-bounds")
        <<"\",\"value\":"<<(forestBoundsAvailable?(targetBehindFaces?"true":"false"):"null")<<"},"
        <<"\"receiverEnvelopeBlocked\":{\"status\":\""<<(forestBoundsAvailable?"evaluated":"unavailable-invalid-selection-bounds")
        <<"\",\"value\":"<<(forestBoundsAvailable?(receiverEnvelope?"true":"false"):"null")<<"},"
        <<"\"transmissionOwnerRoster\":{\"status\":\"evaluated\",\"value\":"
        <<(source->TransmissionOwnersWithin(std::array<std::uint32_t,2>{{8u,PresentableTinyRtScene::kWaterDropletMetadataIndex}})?"true":"false")<<"},"
        <<"\"actualSceneClosureObservation\":{\"status\":\""<<(actualClosureAvailable?"observed":"unavailable")
        <<"\",\"sourceGeneration\":"<<(actualClosureAvailable?std::to_string(actualClosure.sourceGeneration):"null")
        <<",\"fullSourceAvailable\":"
        <<(actualClosureAvailable?(actualClosure.fullSourceAvailable?"true":"false"):"null")
        <<",\"completeContributors\":"<<(actualClosureAvailable?(actualClosure.completeContributors?"true":"false"):"null")
        <<",\"receiverEnvelopeBlocked\":"<<(actualClosureAvailable?(actualClosure.receiverEnvelopeBlocked?"true":"false"):"null")
        <<",\"transmissionOwnersWithin\":"<<(actualClosureAvailable?(actualClosure.transmissionOwnersWithin?"true":"false"):"null")<<"},"
        <<"\"actualRetainedOpaqueFaces\":{\"status\":\""<<(actualClosureAvailable?"evaluated":"unavailable")
        <<"\",\"value\":"<<(actualClosureAvailable?(actualClosure.faceCoverageVerified?"true":"false"):"null")<<"},"
        <<"\"actualRegionalWaterClosure\":{\"status\":\""
        <<(!actualClosureAvailable?"unavailable":!actualClosure.regionalWaterEnabled?"not-required-disabled":
           actualClosure.regionalWaterCheckPerformed?"evaluated":"unavailable-not-performed")
        <<"\",\"enabled\":"<<(actualClosureAvailable?(actualClosure.regionalWaterEnabled?"true":"false"):"null")
        <<",\"performed\":"<<(actualClosureAvailable?(actualClosure.regionalWaterCheckPerformed?"true":"false"):"null")
        <<",\"actualFaceCount\":"<<(actualClosureAvailable?std::to_string(actualClosure.regionalWaterFaceCount):"null")<<",\"value\":"
        <<(actualClosureAvailable && actualClosure.regionalWaterCheckPerformed?(actualClosure.regionalWaterSeparated?"true":"false"):"null")<<"},"
        <<"\"actualLanternTransportBounds\":{\"status\":\""<<(actualClosureAvailable?"evaluated":"unavailable")
        <<"\",\"value\":"<<(actualClosureAvailable?(actualClosure.lanternTransportBounded?"true":"false"):"null")<<"},"
        <<"\"currentFrameEligibility\":{\"status\":\"not-evaluated\",\"value\":null,\"reason\":\"export is not ordinary lease scheduling, current-frame TLAS or source-prism validation\"}}}}\n";
    if(context.rtScene.StaticGeometryGeneration()!=inspectedGeneration ||
       context.rtScene.DebugCachedCompleteStaticWorldSource()!=source)
        return reject("Static inspection source identity changed during export.");
    const auto metadata=json.str();
    if(!WriteStaticWorldInspectionFile(output/"static-world-source.json",
        std::as_bytes(std::span<const char>{metadata.data(),metadata.size()}),error))return false;
    error.clear();return true;
}

bool InspectStaticWorldSource(const VulkanSurfaceContext& context,const std::filesystem::path& directory,
    const std::string& selectedMaterial,std::string& error)
{
    return InspectStaticWorldSourceImpl(context,directory,selectedMaterial,error,nullptr);
}

// Runs the same immutable-source validation, selection and hashes as export,
// without resolving output paths or touching files. On false, no result is
// published. On true, callers must still check structurallyValid and compare
// the fingerprint with an independently reviewed descriptor before engine gates.
bool MeasureStaticWorldForestSource(const VulkanSurfaceContext& context,
    StaticWorldForestMeasurement& measurement,std::string& error)
{
    measurement={};
    return InspectStaticWorldSourceImpl(context,{},"ForestPine",error,&measurement);
}
