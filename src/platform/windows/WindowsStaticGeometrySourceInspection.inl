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

bool InspectStaticWorldSource(const VulkanSurfaceContext& context,const std::filesystem::path& directory,
    const std::string& selectedMaterial,std::string& error)
{
    using namespace horde::vulkan::raytracing;
    using Vertex=horde::scene::assets::StaticRtVertex;
    using Region=horde::scene::assets::StaticTriangleRegion;
    static_assert(sizeof(Vertex)==64u && sizeof(Region)==1u && sizeof(std::uint32_t)==4u);
    const auto reject=[&](const char* reason){error=reason;return false;};
    const auto& launch=ParseCaptureLaunchOptions();
    const auto source=context.rtScene.DebugCachedCompleteStaticWorldSource();
    const auto* asset=context.rtScene.DebugCollapseStaticAsset();
    if(!launch.captureHidden || launch.developmentCheckpoint.empty() || !context.useRtPath || !source || !asset ||
       selectedMaterial.empty() || selectedMaterial.size()>128u || !context.rtScene.StaticGeometryHasCompleteContributors())
        return reject("Static inspection requires the hidden ready chapter's complete cached source.");
    std::error_code pathError;
#if defined(HORDE_RT_SOURCE_DIR)
    const auto reports=std::filesystem::weakly_canonical(std::filesystem::path(HORDE_RT_SOURCE_DIR)/"reports",pathError);
#else
    return reject("Static inspection requires a bound source checkout for ignored output.");
    const std::filesystem::path reports;
#endif
    if(pathError)return reject("Static inspection could not resolve the ignored reports root.");
    const auto output=std::filesystem::weakly_canonical(directory,pathError);
    if(pathError)return reject("Static inspection could not resolve its output directory.");
    const auto relative=output.lexically_relative(reports);
    if(relative.empty() || relative.is_absolute() || output==reports || *relative.begin()=="..")
        return reject("Static inspection output must be a child of this workspace's reports directory.");
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
        <<context.rtScene.StaticGeometryGeneration()<<",\"completeContributors\":true,\"owner\":{\"instance\":"<<ownerIndex
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
    json<<"]}}\n";
    const auto metadata=json.str();
    if(!WriteStaticWorldInspectionFile(output/"static-world-source.json",
        std::as_bytes(std::span<const char>{metadata.data(),metadata.size()}),error))return false;
    error.clear();return true;
}
