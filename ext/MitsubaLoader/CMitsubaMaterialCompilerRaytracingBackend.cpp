#include "../../ext/MitsubaLoader/CMitsubaMaterialCompilerRaytracingBackend.h"

namespace irr
{
namespace asset
{
namespace material_compiler
{

namespace instr_stream
{
	class CInterpreter
	{
	public:
		static const IR::INode* translateMixIntoBlends(IR* ir, const IR::INode* _mix);

		static std::pair<instr_t, const IR::INode*> processSubtree(IR* ir, const IR::INode* tree, IR::INode::children_array_t& next);
	};

	template <typename stack_el_t>
	class ITraveralGenerator
	{
	protected:
		SContext* m_ctx;
		IR* m_ir;

		core::stack<stack_el_t> m_stack;

		template <typename ...Params>
		static stack_el_t createStackEl(Params&& ...args)
		{
			return stack_el_t {std::forward<Params>(args)...};
		}

		virtual void writeInheritableBitfields(instr_t& dst, instr_t parent) const
		{
			dst = core::bitfieldInsert(dst, parent, BITFIELDS_SHIFT_TWOSIDED, BITFIELDS_WIDTH_TWOSIDED);
			dst = core::bitfieldInsert(dst, parent, BITFIELDS_SHIFT_MASKFLAG, BITFIELDS_WIDTH_MASKFLAG);
		}

		virtual void writeBumpmapBitfields(instr_t& dst)
		{
			//stuff specific for bumpmaps
		}

		void filterNOOPs(traversal_t& _traversal)
		{
			_traversal.erase(
				std::remove_if(_traversal.begin(), _traversal.end(), [](instr_t i) { return getOpcode(i) == OP_NOOP; }),
				_traversal.end()
			);
		}

		std::pair<instr_t, const IR::INode*> processSubtree(const IR::INode* tree, IR::INode::children_array_t& next)
		{
			//TODO deduplication
			return CInterpreter::processSubtree(m_ir, tree, next);
		}

		void setBSDFData(SBSDFUnion& _dst, E_OPCODE _op, const IR::INode* _node) const
		{
			switch (_op)
			{
			case OP_DIFFUSE:
			{
				auto* node = static_cast<const IR::CMicrofacetDiffuseBSDFNode*>(_node);
				if (node->alpha_u.source == IR::INode::EPS_TEXTURE)
					_dst.diffuse.alpha.setTexture(packTexture(node->alpha_u.value.texture), node->alpha_u.value.texture.scale);
				else
					_dst.diffuse.alpha.setConst(node->alpha_u.value.constant);
				if (node->reflectance.source == IR::INode::EPS_TEXTURE)
					_dst.diffuse.reflectance.setTexture(packTexture(node->reflectance.value.texture), node->reflectance.value.texture.scale);
				else
					_dst.diffuse.reflectance.setConst(node->reflectance.value.constant.pointer);
			}
			break;
			case OP_DIELECTRIC:
			{
				auto* blend = static_cast<const IR::CBSDFCombinerNode*>(_node);
				auto* refl = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(blend->children[0]);
				auto* trans = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(blend->children[1]);
				if (refl->scatteringMode == IR::CMicrofacetSpecularBSDFNode::ESM_TRANSMIT)
					std::swap(refl, trans);
				assert(refl->ndf == trans->ndf);
				assert(refl->alpha_u == trans->alpha_u);
				assert(refl->alpha_v == trans->alpha_v);

				if (refl->alpha_u.source == IR::INode::EPS_TEXTURE)
					_dst.dielectric.alpha_u.setTexture(packTexture(refl->alpha_u.value.texture), refl->alpha_u.value.texture.scale);
				else
					_dst.dielectric.alpha_u.setConst(refl->alpha_u.value.constant);
				if (refl->ndf == IR::CMicrofacetSpecularBSDFNode::ENDF_ASHIKHMIN_SHIRLEY)
				{
					if (refl->alpha_v.source == IR::INode::EPS_TEXTURE)
						_dst.dielectric.alpha_v.setTexture(packTexture(refl->alpha_v.value.texture), refl->alpha_v.value.texture.scale);
					else
						_dst.dielectric.alpha_v.setConst(refl->alpha_v.value.constant);
				}
			}
			break;
			case OP_CONDUCTOR:
			{
				auto* node = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(_node);
				
				if (node->alpha_u.source == IR::INode::EPS_TEXTURE)
					_dst.conductor.alpha_u.setTexture(packTexture(node->alpha_u.value.texture), node->alpha_u.value.texture.scale);
				else
					_dst.conductor.alpha_u.setConst(node->alpha_u.value.constant);
				if (node->ndf == IR::CMicrofacetSpecularBSDFNode::ENDF_ASHIKHMIN_SHIRLEY)
				{
					if (node->alpha_v.source == IR::INode::EPS_TEXTURE)
						_dst.conductor.alpha_v.setTexture(packTexture(node->alpha_v.value.texture), node->alpha_v.value.texture.scale);
					else
						_dst.conductor.alpha_v.setConst(node->alpha_v.value.constant);
				}
				_dst.conductor.eta[0] = core::rgb32f_to_rgb19e7(node->eta.pointer);
				_dst.conductor.eta[1] = core::rgb32f_to_rgb19e7(node->etaK.pointer);
			}
			break;
			case OP_PLASTIC:
			{
				auto* node = static_cast<const IR::CBSDFCombinerNode*>(_node);
				assert(node->type == IR::CBSDFCombinerNode::ET_DIFFUSE_AND_SPECULAR);

				auto* d = static_cast<const IR::CBSDFNode*>(node->children[0]);
				auto* s = static_cast<const IR::CBSDFNode*>(node->children[1]);
				if (d->type == IR::CBSDFNode::ET_MICROFACET_SPECULAR)
					std::swap(d, s);

				auto* diffuse = static_cast<const IR::CMicrofacetDiffuseBSDFNode*>(d);
				auto* specular = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(s);

				if (diffuse->reflectance.source == IR::INode::EPS_TEXTURE)
					_dst.plastic.alpha.setTexture(packTexture(diffuse->reflectance.value.texture), diffuse->reflectance.value.texture.scale);
				else
					_dst.plastic.alpha.setConst(diffuse->reflectance.value.constant.pointer);
				if (diffuse->alpha_u.source == IR::INode::EPS_TEXTURE)
					_dst.plastic.alpha.setTexture(packTexture(diffuse->alpha_u.value.texture), diffuse->alpha_u.value.texture.scale);
				else
					_dst.plastic.alpha.setConst(diffuse->alpha_u.value.constant);

				_dst.plastic.eta = diffuse->eta.x;
			}
			break;
			case OP_COATING:
			{
				auto* coat = static_cast<const IR::CCoatingBSDFNode*>(_node);

				if (coat->alpha_u.source == IR::INode::EPS_TEXTURE)
					_dst.coating.alpha.setTexture(packTexture(coat->alpha_u.value.texture), coat->alpha_u.value.texture.scale);
				else
					_dst.coating.alpha.setConst(coat->alpha_u.value.constant);
				if (coat->sigmaA.source == IR::INode::EPS_TEXTURE)
					_dst.coating.sigmaA.setTexture(packTexture(coat->alpha_u.value.texture), coat->sigmaA.value.texture.scale);
				else
					_dst.coating.sigmaA.setConst(coat->sigmaA.value.constant.pointer);

				uint32_t thickness_eta;
				thickness_eta = core::Float16Compressor::compress(coat->thickness);
				thickness_eta |= static_cast<uint32_t>(core::Float16Compressor::compress(coat->eta.x))<<16;
				_dst.coating.thickness_eta = thickness_eta;
			}
			break;
			case OP_BLEND:
			{
				auto* b = static_cast<const IR::CBSDFCombinerNode*>(_node);
				assert(b->type == IR::CBSDFCombinerNode::ET_WEIGHT_BLEND);
				auto* blend = static_cast<const IR::CBSDFBlendNode*>(b);

				if (blend->weight.source == IR::INode::EPS_TEXTURE)
					_dst.blend.weight.setTexture(packTexture(blend->weight.value.texture), blend->weight.value.texture.scale);
				else
					_dst.blend.weight.setConst(blend->weight.value.constant);
			}
			break;
			case OP_DIFFTRANS:
			{
				auto* difftrans = static_cast<const IR::CDifftransBSDFNode*>(_node);

				if (difftrans->transmittance.source == IR::INode::EPS_TEXTURE)
					_dst.difftrans.transmittance.setTexture(packTexture(difftrans->transmittance.value.texture), difftrans->transmittance.value.texture.scale);
				else
					_dst.difftrans.transmittance.setConst(difftrans->transmittance.value.constant.pointer);
			}
			case OP_BUMPMAP:
			{
				const IR::CGeomModifierNode* bm = nullptr;
				if (_node->symbol == IR::INode::ES_MATERIAL)
				{
					size_t ix = 0u;
					if (_node->children.find(IR::INode::ES_GEOM_MODIFIER, &ix))
						bm = static_cast<const IR::CGeomModifierNode*>(_node->children[ix]);
				}
				else if (_node->symbol == IR::INode::ES_GEOM_MODIFIER)
					bm = static_cast<const IR::CGeomModifierNode*>(_node);

				_dst.bumpmap.bumpmap.vtid = bm ? packTexture(bm->texture) : VTID::invalid();
				core::uintBitsToFloat(_dst.bumpmap.bumpmap.scale) = bm ? bm->texture.scale : 0.f;
			}
			break;
			}
		}

		size_t getBSDFDataIndex(E_OPCODE _op, const IR::INode* _node) const
		{
			switch (_op)
			{
			case OP_INVALID: _IRR_FALLTHROUGH;
			case OP_NOOP:
				return 0ull;
			default: break;
			}

			auto found = m_ctx->bsdfDataIndexMap.find(_node);
			if (found != m_ctx->bsdfDataIndexMap.end())
				return found->second;

			SBSDFUnion data;
			setBSDFData(data, _op, _node);
			size_t ix = m_ctx->bsdfData.size();
			m_ctx->bsdfDataIndexMap.insert({_node,ix});
			m_ctx->bsdfData.push_back(data);

			return ix;
		}

		VTID packTexture(const IR::INode::STextureSource& tex)
		{
			auto found = m_ctx->VTallocMap.find({tex.image.get(),tex.sampler.get()});
			if (found != m_ctx->VTallocMap.end())
				return found->second;

			auto* img = tex.image->getCreationParameters().image.get();
			auto* sampler = tex.sampler.get();

			const auto& extent = img->getCreationParameters().extent;
			const auto uwrap = static_cast<asset::ISampler::E_TEXTURE_CLAMP>(sampler->getParams().TextureWrapU);
			const auto vwrap = static_cast<asset::ISampler::E_TEXTURE_CLAMP>(sampler->getParams().TextureWrapV);
			const auto border = static_cast<asset::ISampler::E_TEXTURE_BORDER_COLOR>(sampler->getParams().BorderColor);

			auto imgAndOrigSz = asset::ICPUVirtualTexture::createPoTPaddedSquareImageWithMipLevels(img, uwrap, vwrap, border);

			asset::IImage::SSubresourceRange subres;
			subres.baseArrayLayer = 0u;
			subres.layerCount = 1u;
			subres.baseMipLevel = 0u;
			auto mx = std::max(extent.width, extent.height);
			auto round = core::roundUpToPoT<uint32_t>(mx);
			auto lsb = core::findLSB(round);
			subres.levelCount = lsb + 1;

			auto addr = m_ctx->vt->alloc(img->getCreationParameters().format, imgAndOrigSz.second, subres, uwrap, vwrap);
			m_ctx->vt->commit(addr, imgAndOrigSz.first.get(), subres, uwrap, vwrap, border);

			std::pair<SContext::VTallocKey, VTID> item{{tex.image.get(),tex.sampler.get()}, addr};
			m_ctx->VTallocMap.insert(item);

			return addr;
		}

		template <typename ...Params>
		bool push(const instr_t _instr, const IR::INode* _node, const IR::INode::children_array_t& _children, instr_t _parent, Params&& ...args)
		{
			bool pushIt = false;
			instr_t instr = _instr;
			writeInheritableBitfields(instr, _parent);
			switch (getOpcode(instr))
			{
			case OP_BUMPMAP:
				writeBumpmapBitfields(instr);
				_IRR_FALLTHROUGH;
			case OP_DIFFUSE: _IRR_FALLTHROUGH;
			case OP_DIFFTRANS: _IRR_FALLTHROUGH;
			case OP_DIELECTRIC: _IRR_FALLTHROUGH;
			case OP_CONDUCTOR: _IRR_FALLTHROUGH;
			case OP_PLASTIC: _IRR_FALLTHROUGH;
			case OP_COATING: _IRR_FALLTHROUGH;
			case OP_BLEND: _IRR_FALLTHROUGH;
			case OP_NOOP:
				pushIt = true;
				break;
			}
			if (pushIt)
				m_stack.push(createStackEl(instr, _node, std::forward<Params>(args)...));

			return pushIt;
		}
		instr_t finalizeInstr(instr_t _instr, const IR::INode* _node, uint32_t _bsdfBufOffset)
		{
			constexpr E_NDF ndfMap[4]
			{
				NDF_BECKMANN,
				NDF_GGX,
				NDF_AS,
				NDF_PHONG
			};

			auto handleSpecularBitfields = [](instr_t dst, const IR::CMicrofacetSpecularBSDFNode* node) -> instr_t
			{
				dst = core::bitfieldInsert<instr_t>(dst, ndfMap[node->ndf], BITFIELDS_SHIFT_NDF, BITFIELDS_WIDTH_NDF);
				if (ndfMap[node->ndf] == NDF_AS && node->alpha_v.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(dst, 1u, BITFIELDS_SHIFT_ALPHA_V_TEX, 1);
				dst = core::bitfieldInsert<instr_t>(dst, 1u, BITFIELDS_MASK_ALPHA_U_TEX, 1);

				return dst;
			};

			const E_OPCODE op = getOpcode(_instr);
			switch (op)
			{
			case OP_DIFFUSE:
			{
				auto* node = static_cast<const IR::CMicrofacetDiffuseBSDFNode*>(_node);
				if (node->alpha_u.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_ALPHA_U_TEX, 1);
				if (node->reflectance.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_REFL_TEX, 1);
			}
			break;
			case OP_DIELECTRIC:
			{
				auto* blend = static_cast<const IR::CBSDFCombinerNode*>(_node);
				auto* refl = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(blend->children[0]);
				auto* trans = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(blend->children[1]);
				if (refl->scatteringMode == IR::CMicrofacetSpecularBSDFNode::ESM_TRANSMIT)
					std::swap(refl, trans);
				assert(refl->ndf == trans->ndf);
				assert(refl->alpha_u == trans->alpha_u);
				assert(refl->alpha_v == trans->alpha_v);

				_instr = handleSpecularBitfields(_instr, refl);
			}
			break;
			case OP_CONDUCTOR:
			{
				auto* node = static_cast<const IR::CMicrofacetSpecularBSDFNode*>(_node);
				_instr = handleSpecularBitfields(_instr, node);
			}
			break;
			case OP_PLASTIC:
			{
				auto* node = static_cast<const IR::CBSDFCombinerNode*>(_node);
				assert(node->type == IR::CBSDFCombinerNode::ET_DIFFUSE_AND_SPECULAR);

				auto* diffuse = static_cast<const IR::CBSDFNode*>(node->children[0]);
				auto* specular = static_cast<const IR::CBSDFNode*>(node->children[1]);
				if (diffuse->type == IR::CBSDFNode::ET_MICROFACET_SPECULAR)
					std::swap(diffuse, specular);

				_instr = handleSpecularBitfields(static_cast<const IR::CMicrofacetSpecularBSDFNode*>(specular));
				if (static_cast<const IR::CMicrofacetDiffuseBSDFNode*>(diffuse)->reflectance.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_REFL_TEX, 1);
			}
			break;
			case OP_COATING:
			{
				auto* coat = static_cast<const IR::CCoatingBSDFNode*>(_node);

				_instr = handleSpecularBitfields(coat);
				if (coat->sigmaA.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_SIGMA_A_TEX, 1);
			}
			break;
			case OP_BLEND:
			{
				auto* blend = static_cast<const IR::CBSDFCombinerNode*>(_node);
				assert(blend->type == IR::CBSDFCombinerNode::ET_WEIGHT_BLEND);

				if (static_cast<const IR::CBSDFBlendNode*>(_node)->weight.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_WEIGHT_TEX, 1);
			}
			case OP_DIFFTRANS:
			{
				auto* difftrans = static_cast<const IR::CDifftransBSDFNode*>(_node);

				if (difftrans->transmittance.source == IR::INode::EPS_TEXTURE)
					_instr = core::bitfieldInsert<instr_t>(_instr, 1u, BITFIELDS_SHIFT_TRANS_TEX, 1);
			}
			break;
			}

			_instr = core::bitfieldInsert<instr_t>(_instr, _bsdfBufOffset, BITFIELDS_BSDF_BUF_OFFSET_SHIFT, BITFIELDS_BSDF_BUF_OFFSET_WIDTH);

			return _instr;
		}

	public:
		ITraveralGenerator(SContext* _ctx, IR* _ir) : m_ctx(_ctx), m_ir(_ir) {}

		virtual traversal_t genTraversal(const IR::INode* _root) = 0;

#ifdef _IRR_DEBUG
		static void debugPrint(const traversal_t& _traversal)
		{
			const char* names[OPCODE_COUNT]{
				"OP_DIFFUSE",
				"OP_CONDUCTOR",
				"OP_PLASTIC",
				"OP_COATING",
				"OP_DIFFTRANS",
				"OP_DIELECTRIC",
				"OP_BLEND",
				"OP_BUMPMAP",
				"OP_SET_GEOM_NORMAL",
				"OP_INVALID",
				"OP_NOOP"
			};
			auto regsString = [](const core::vector3du32_SIMD& regs, uint32_t usedSrcs) {
				std::string s;
				if (usedSrcs)
				{
					s += "(";
					s += std::to_string(regs.y);
					if (usedSrcs > 1u)
						s += "," + std::to_string(regs.z);
					s += ")";
				}
				return s += "->" + std::to_string(regs.x);
			};

			for (const auto& i : _traversal)
			{
				const auto op = getOpcode(i);
				std::string s = names[op];
				s += " :\t\t" + regsString(getRegisters(i), getNumberOfSrcRegsForOpcode(op));
				if (isTwosided(i))
					s += "\t\tTS";
				if (isMasked(i))
					s += "\t\tM";
				os::Printer::log(s, ELL_DEBUG);
			}
		}
#endif
	};

namespace remainder_and_pdf
{
	inline uint32_t getNumberOfSrcRegsForOpcode(E_OPCODE _op)
	{
		if (_op == OP_BLEND)
			return 2u;
		else if (_op == OP_BUMPMAP || _op == OP_COATING)
			return 1u;
		return 0u;
	}

	inline uint32_t getNormalId(const instr_t& i)
	{
		return i >> INSTR_NORMAL_ID_SHIFT;
	}
	inline core::vector3du32_SIMD getRegisters(const instr_t& i)
	{
		return core::vector3du32_SIMD(
			(i>>INSTR_REG_DST_SHIFT),
			(i>>INSTR_REG_SRC1_SHIFT),
			(i>>INSTR_REG_SRC2_SHIFT)
		) & core::vector3du32_SIMD(INSTR_REG_MASK);
	}

	class CTraversalManipulator
	{
	public:
		using traversal_t = core::vector<instr_t>;
		using substream_t = std::pair<traversal_t::iterator, traversal_t::iterator>;

	private:
		_IRR_STATIC_INLINE_CONSTEXPR instr_t SPECIAL_VAL = ~static_cast<instr_t>(0);

		static void setRegisters(instr_t& i, uint32_t rdst, uint32_t rsrc1 = 0u, uint32_t rsrc2 = 0u)
		{
			uint32_t& regDword = reinterpret_cast<uint32_t*>(&i)[1];

			constexpr uint32_t _2ND_DWORD_SHIFT = 32u;

			regDword = core::bitfieldInsert(regDword, rdst, INSTR_REG_DST_SHIFT-_2ND_DWORD_SHIFT, INSTR_REG_WIDTH);
			regDword = core::bitfieldInsert(regDword, rsrc1, INSTR_REG_SRC1_SHIFT-_2ND_DWORD_SHIFT, INSTR_REG_WIDTH);
			regDword = core::bitfieldInsert(regDword, rsrc2, INSTR_REG_SRC2_SHIFT-_2ND_DWORD_SHIFT, INSTR_REG_WIDTH);
		}

		struct has_different_normal_id
		{
			uint32_t n_id;

			bool operator()(const instr_t& i) const { return getNormalId(i) != n_id; };
		};

		traversal_t m_input;
		core::queue<uint32_t> m_streamLengths;

		void reorderBumpMapStreams_impl(traversal_t& _input, traversal_t& _output, const substream_t& _stream);

	public:
		CTraversalManipulator(traversal_t&& _traversal) : m_input(std::move(_traversal)) {}

		traversal_t&& process(uint32_t regCount)&&
		{
			reorderBumpMapStreams();
			specifyRegisters(regCount);

			return std::move(m_input);
		}

	private:
		//reorders scattered bump-map streams (traversals of BSDF subtrees below bumpmap BSDF node) into continuous streams
		//and moves OP_BUMPMAPs to the beginning of their streams/traversals/subtrees (because obviously they must be executed before BSDFs using them)
		//leaves SPECIAL_VAL to mark original places of beginning of a stream (needed for function specifying registers)
		//WARNING: modifies m_input
		void reorderBumpMapStreams()
		{
			traversal_t result;
			reorderBumpMapStreams_impl(m_input, result, { m_input.begin(),m_input.end() });

			m_input = std::move(result);
		}

		void specifyRegisters(uint32_t regCount);
	};


	struct stack_el
	{
		instr_t instr;
		const IR::INode* node;
		IR::INode::children_array_t children;
		bool visited;
	};
	class CTraversalGenerator : public ITraveralGenerator<stack_el>
	{
		using base_t = ITraveralGenerator<stack_el>;

		uint32_t m_firstFreeNormalID = 0u;

	public:
		using base_t::base_t;

		void writeInheritableBitfields(instr_t& dst, instr_t parent) const override
		{
			base_t::writeInheritableBitfields(dst, parent);
			
			dst = core::bitfieldInsert(dst, parent, INSTR_NORMAL_ID_SHIFT, INSTR_NORMAL_ID_WIDTH);
		}

		virtual void writeBumpmapBitfields(instr_t& dst) override
		{
			dst = core::bitfieldInsert<instr_t>(dst, m_firstFreeNormalID, INSTR_NORMAL_ID_SHIFT, INSTR_NORMAL_ID_WIDTH);
			++m_firstFreeNormalID;
		}

		traversal_t genTraversal(const IR::INode* _root) override;
	};
}

namespace gen_choice
{
	struct stack_el
	{
		instr_t instr;
		const IR::INode* node;
		IR::INode::children_array_t children;
		uint32_t parentIx;
	};
	class CTraversalGenerator : public ITraveralGenerator<stack_el>
	{
		using base_t = ITraveralGenerator<stack_el>;

		uint32_t m_firstFreeNormalID = 0u;

	public:
		using base_t::base_t;

		traversal_t genTraversal(const IR::INode* _root) override;
	};
}
namespace tex_prefetch
{
	static traversal_t genTraversal(const traversal_t& _t, const core::vector<instr_stream::SBSDFUnion>& _bsdfData, core::unordered_map<STextureData, uint32_t, STextureData::hash>& _tex2reg);
}
}


using namespace instr_stream;

const IR::INode* instr_stream::CInterpreter::translateMixIntoBlends(IR* ir, const IR::INode* _mix)
{
	assert(mix->symbol == IR::INode::ES_BSDF_COMBINER);
	auto* mix = static_cast<const IR::CBSDFMixNode*>(_mix);

	struct q_el
	{
		const IR::INode* node;
		float weightsSum;
	};
	core::queue<q_el> q;
	for (uint32_t i = 0u; i < mix->children.count; ++i)
		q.push({mix->children[i],mix->weights[i]});

	while (q.size()>1ull)
	{
		auto* blend = ir->allocNode<IR::CBSDFBlendNode>();
		blend->weight.source = IR::INode::EPS_CONSTANT;

		auto left = q.front();
		q.pop();
		auto right = q.front();
		q.pop();

		q_el el{blend, left.weightsSum+right.weightsSum};
		blend->weight.value.constant = right.weightsSum/el.weightsSum;
		blend->children = IR::INode::createChildrenArray(left.node,right.node);

		q.push(el);
	}

	return q.front().node;
}

std::pair<instr_t, const IR::INode*> instr_stream::CInterpreter::processSubtree(IR* ir, const IR::INode* tree, IR::INode::children_array_t& next)
{
	auto isTwosided = [](const IR::INode* _node, size_t _frontSurfIx, IR::INode::children_array_t& _frontSurfChildren) -> bool
	{
		bool ts = false;
		auto* front = _node->children[_frontSurfIx];
		if (_node->children.find(IR::INode::ES_BACK_SURFACE, &_frontSurfIx))
		{
			auto* back = _node->children[_frontSurfIx];
			assert(front->children == back->children);//TODO handle this in some other way
			ts = true;
		}
		_frontSurfChildren = front->children;

		return ts;
	};

	instr_t instr = OP_INVALID;
	switch (tree->symbol)
	{
	case IR::INode::ES_MATERIAL:
	{
		bool twosided = false;
		bool masked = false;

		auto* node = static_cast<const IR::CMaterialNode*>(tree);
		masked = (node->opacity.source==IR::INode::EPS_TEXTURE || (node->opacity.source==IR::INode::EPS_TEXTURE && (node->opacity.value.constant!=IR::INode::color_t(1.f)).xyzz().all()));

		instr_t retval = OP_INVALID;

		size_t ix = 0u;
		if (node->children.find(IR::INode::ES_GEOM_MODIFIER, &ix))
		{
			auto* geom_node = static_cast<const IR::CGeomModifierNode*>(node->children[ix]);
			next = geom_node->children;
			retval = (geom_node->type != IR::CGeomModifierNode::ET_HEIGHT) ? OP_INVALID : OP_BUMPMAP;
		}
		else if (node->children.find(IR::INode::ES_FRONT_SURFACE, &ix))
		{
			twosided = isTwosided(node, ix, next);
			retval = OP_NOOP; //returning OP_NOOP causes not adding this instruction to resulting stream, but doesnt stop processing (effectively is used to forward flags only)
		}
		if (twosided)
			retval = core::bitfieldInsert<instr_t>(retval, 1u, BITFIELDS_SHIFT_TWOSIDED, 1);
		if (masked)
			retval = core::bitfieldInsert<instr_t>(retval, 1u, BITFIELDS_SHIFT_MASKFLAG, 1);
		instr = retval;
	}
	break;
	case IR::INode::ES_GEOM_MODIFIER:
	{
		bool twosided = false;
		auto* node = static_cast<const IR::CGeomModifierNode*>(tree);
		size_t ix;
		if (node->children.find(IR::INode::ES_FRONT_SURFACE, &ix))
			twosided = isTwosided(node, ix, next);

		instr_t retval;
		if (node->type == IR::CGeomModifierNode::ET_HEIGHT)
			retval = OP_BUMPMAP;
		else
			retval = OP_INVALID;

		if (twosided)
			retval = core::bitfieldInsert<instr_t>(retval, 1u, BITFIELDS_SHIFT_TWOSIDED, 1);

		instr = retval;
	}
	break;
	case IR::INode::ES_BSDF_COMBINER:
	{
		auto* node = static_cast<const IR::CBSDFCombinerNode*>(tree);
		switch (node->type)
		{
		case IR::CBSDFCombinerNode::ET_WEIGHT_BLEND:
			next = node->children;
			instr = OP_BLEND;
			break;
		case IR::CBSDFCombinerNode::ET_DIFFUSE_AND_SPECULAR:
		{
			assert(node->children.count==2u);
			assert(node->children[0]->symbol==IR::INode::ES_BSDF && node->children[1]->symbol==IR::INode::ES_BSDF);
			auto type = static_cast<const IR::CBSDFNode*>(node->children[0])->type;
			assert(type==IR::CBSDFNode::ET_MICROFACET_DIFFUSE || type==IR::CBSDFNode::ET_MICROFACET_SPECULAR);
			type = static_cast<const IR::CBSDFNode*>(node->children[1])->type;
			assert(type==IR::CBSDFNode::ET_MICROFACET_DIFFUSE || type==IR::CBSDFNode::ET_MICROFACET_SPECULAR);
			instr = OP_PLASTIC;
		}
		break;
		case IR::CBSDFCombinerNode::ET_FRESNEL_BLEND:
		{
			assert(node->children.count==2u);
			assert(node->children[0]->symbol==IR::INode::ES_BSDF && node->children[1]->symbol==IR::INode::ES_BSDF);
			auto type = static_cast<const IR::CBSDFNode*>(node->children[0])->type;
			assert(type == IR::CBSDFNode::ET_MICROFACET_SPECULAR);
			type = static_cast<const IR::CBSDFNode*>(node->children[1])->type;
			assert(type == IR::CBSDFNode::ET_MICROFACET_SPECULAR);
			instr = OP_DIELECTRIC;
		}
		break;
		case IR::CBSDFCombinerNode::ET_MIX:
		{
			tree = translateMixIntoBlends(ir, node);
			instr = OP_BLEND;
			next = tree->children;
		}
		break;
		}
	}
	break;
	case IR::INode::ES_BSDF:
	{
		auto* node = static_cast<const IR::CBSDFNode*>(tree);
		switch (node->type)
		{
		case IR::CBSDFNode::ET_DIFFTRANS:
			instr = OP_DIFFTRANS; break;
		case IR::CBSDFNode::ET_MICROFACET_DIFFUSE:
			instr = OP_DIFFUSE; break;
		case IR::CBSDFNode::ET_MICROFACET_SPECULAR:
			instr = OP_CONDUCTOR; break;
		case IR::CBSDFNode::ET_COATING:
			next = node->children;
			assert(node->children.count==1u);
			instr = OP_COATING;
			break;
		}
	}
	break;
	}

	return {instr,tree};
}

void instr_stream::remainder_and_pdf::CTraversalManipulator::reorderBumpMapStreams_impl(traversal_t& _input, traversal_t& _output, const substream_t& _stream)
{
	const uint32_t n_id = getNormalId(*(_stream.second - 1));

	const size_t len = _stream.second - _stream.first;
	size_t subsLenAcc = 0ull;

	core::stack<substream_t> substreams;
	auto subBegin = std::find_if(_stream.first, _stream.second, has_different_normal_id{ n_id });
	while (subBegin != _stream.second)
	{
		const uint32_t sub_n_id = getNormalId(*subBegin);
		decltype(subBegin) subEnd;
		for (subEnd = _stream.second - 1; subEnd != subBegin; --subEnd)
			if (*subEnd == sub_n_id)
				break;
		++subEnd;

		//one place will be used for SPECIAL_VAL (hence -1)
		subsLenAcc += subEnd - subBegin - 1;
		substreams.push({ subBegin,subEnd });

		subBegin = std::find_if(subEnd, _stream.second, has_different_normal_id{ n_id });
	}

	while (!substreams.empty())
	{
		reorderBumpMapStreams_impl(_input, _output, substreams.top());
		substreams.pop();
	}

	const uint32_t newlen = len - subsLenAcc;

	substream_t woSubs{ _stream.first,_stream.first + newlen };
	//move bumpmap instruction to the beginning of the stream
	auto lastInstr = *(_stream.first + newlen - 1);
	if (getOpcode(lastInstr) == OP_BUMPMAP)
	{
		_input.erase(_stream.first + newlen - 1);
		_input.insert(_stream.first, lastInstr);
	}

	//important for next stage of processing
	m_streamLengths.push(newlen);

	_output.insert(_output.end(), woSubs.first, woSubs.second);
	*woSubs.first = SPECIAL_VAL;
	//do not erase SPECIAL_VAL (hence +1)
	_input.erase(woSubs.first + 1, woSubs.second);
}

void instr_stream::remainder_and_pdf::CTraversalManipulator::specifyRegisters(uint32_t regCount)
{
	core::stack<uint32_t> freeRegs;
	{
		for (uint32_t i = 0u; i < regCount; ++i)
			freeRegs.push(regCount - 1u - i);
	}
	//registers with result of bump-map substream
	core::stack<uint32_t> bmRegs;
	//registers waiting to be used as source
	core::stack<uint32_t> srcRegs;

	int32_t bmStreamEndCounter = 0;
	auto pushResultRegister = [&bmStreamEndCounter, &bmRegs, &srcRegs](uint32_t _resultReg)
	{
		core::stack<uint32_t>& stack = bmStreamEndCounter == 0 ? bmRegs : srcRegs;
		stack.push(_resultReg);
	};
	for (uint32_t j = 0u; j < m_input.size();)
	{
		instr_t& i = m_input[j];

		if (i == SPECIAL_VAL)
		{
			srcRegs.push(bmRegs.top());
			bmRegs.pop();
			m_input.erase(m_input.begin() + j);

			continue;// do not increment j
		}
		const E_OPCODE op = getOpcode(i);

		--bmStreamEndCounter;
		if (op == OP_BUMPMAP)
		{
			bmStreamEndCounter = m_streamLengths.front() - 2u;
			m_streamLengths.pop();

			//OP_BUMPMAP doesnt care about usual registers, so dont set them
			++j;
			continue;
		}
		//if bmStreamEndCounter reaches value of -1 and next instruction is not OP_BUMPMAP, then emit some SET_GEOM_NORMAL instr
		else if (bmStreamEndCounter == -1)
		{
			//just opcode, no registers nor other bitfields in this instruction
			m_input.insert(m_input.begin() + j, instr_t(OP_SET_GEOM_NORMAL));

			++j;
			continue;
		}

		const uint32_t srcsNum = getNumberOfSrcRegsForOpcode(op);
		assert(srcsNum <= 2u);
		uint32_t srcs[2];
		for (uint32_t k = 0u; k < srcsNum; ++k)
		{
			srcs[k] = srcRegs.top();
			srcRegs.pop();
		}

		switch (srcsNum)
		{
		case 2u:
		{
			const uint32_t src2 = srcs[0];
			const uint32_t src1 = srcs[1];
			const uint32_t dst = (j == m_input.size() - 1u) ? 0u : src1;
			pushResultRegister(dst);
			freeRegs.push(src2);
			setRegisters(i, dst, src1, src2);
		}
		break;
		case 1u:
		{
			const uint32_t src = srcs[0];
			const uint32_t dst = (j == m_input.size() - 1u) ? 0u : src;
			pushResultRegister(dst);
			setRegisters(i, dst, src);
		}
		break;
		case 0u:
		{
			assert(!freeRegs.empty());
			uint32_t dst = 0u;
			if (j < m_input.size() - 1u)
			{
				dst = freeRegs.top();
				freeRegs.pop();
			}
			pushResultRegister(dst);
			setRegisters(i, dst);
		}
		break;
		default: break;
		}

		++j;
	}
}

instr_stream::traversal_t instr_stream::remainder_and_pdf::CTraversalGenerator::genTraversal(const IR::INode* _root)
{
	traversal_t traversal;

	{
		IR::INode::children_array_t next;
		const IR::INode* node;
		instr_t instr;
		std::tie(instr, node) = processSubtree(_root, next);
		push(instr, node, next, 0u, false);
	}
	while (!m_stack.empty())
	{
		auto& top = m_stack.top();
		const E_OPCODE op = getOpcode(top.instr);
		const uint32_t srcRegCount = getNumberOfSrcRegsForOpcode(op);
		_IRR_DEBUG_BREAK_IF(op == OP_INVALID);
		if (srcRegCount == 0u || top.visited)
		{
			const uint32_t bsdfBufIx = getBSDFDataIndex(getOpcode(top.instr), top.node);
			instr_t instr = finalizeInstr(top.instr, top.node, bsdfBufIx);
			traversal.push_back(instr);
			m_stack.pop();
		}
		else if (!top.visited)
		{
			top.visited = true;

			size_t pushedCount = 0ull;
			for (auto it = top.children.end() - 1; it != top.children.begin() - 1; --it)
			{
				const IR::INode* node = nullptr;
				IR::INode::children_array_t next2;
				instr_t instr2;
				std::tie(instr2, node) = processSubtree(*it, next2);
				pushedCount += push(instr2, node, next2, top.instr, false);
			}
			_IRR_DEBUG_BREAK_IF(pushedCount > 2ull);
		}
	}

	//remove NOOPs
	filterNOOPs(traversal);

	traversal = std::move(CTraversalManipulator(std::move(traversal)).process(REGISTER_COUNT));
#ifdef _IRR_DEBUG
	os::Printer::log("BSDF remainder-and-pdf traversal debug print", ELL_DEBUG);
	debugPrint(traversal);
#endif

	return traversal;
}

instr_stream::traversal_t instr_stream::gen_choice::CTraversalGenerator::genTraversal(const IR::INode* _root)
{
	constexpr uint32_t INVALID_INDEX = 0xdeadbeefu;

	traversal_t traversal;

	{
		IR::INode::children_array_t next;
		const IR::INode* node;
		instr_t instr;
		std::tie(instr, node) = processSubtree(_root, next);
		push(instr, node, next, 0u, INVALID_INDEX);
	}
	while (!m_stack.empty())
	{
		auto& top = m_stack.top();
		const E_OPCODE op = getOpcode(top.instr);

		const uint32_t bsdfBufIx = getBSDFDataIndex(op, top.node);
		const uint32_t currIx = traversal.size();
		traversal.push_back(finalizeInstr(top.instr, top.node, bsdfBufIx));
		_IRR_DEBUG_BREAK_IF(op == OP_INVALID);
		if (top.parentIx != INVALID_INDEX)
		{
			instr_t& parent = traversal[top.parentIx];
			parent = core::bitfieldInsert<instr_t>(parent, currIx, INSTR_RIGHT_JUMP_SHIFT, INSTR_RIGHT_JUMP_WIDTH);
		}

		{
			size_t pushedCount = 0ull;
			for (auto it = top.children.end() - 1; it != top.children.begin() - 1; --it)
			{
				const IR::INode* node = nullptr;
				IR::INode::children_array_t next2;
				instr_t instr2;
				std::tie(instr2, node) = processSubtree(*it, next2);
				pushedCount += push(instr2, node, next2, top.instr, it == top.children.begin() + 1 ? currIx : INVALID_INDEX);
			}
			_IRR_DEBUG_BREAK_IF(pushedCount>2ull);
		}
	}

	//remove NOOPs
	filterNOOPs(traversal);

#ifdef _IRR_DEBUG
	os::Printer::log("BSDF generator-choice traversal debug print", ELL_DEBUG);
	debugPrint(traversal);
#endif

	return traversal;
}

instr_stream::traversal_t instr_stream::tex_prefetch::genTraversal(const traversal_t& _t, const core::vector<instr_stream::SBSDFUnion>& _bsdfData, core::unordered_map<instr_stream::STextureData, uint32_t, instr_stream::STextureData::hash>& _tex2reg)
{
	core::unordered_set<STextureData, STextureData::hash> processed;
	uint32_t regNum = 0u;
	auto write_fetch_flag = [&processed,&regNum,&_tex2reg](instr_t& i, const STextureData& tex, int32_t dstbit, int32_t srcbit, int32_t reg_bitfield_shift) {
		auto bit = core::bitfieldExtract(i, srcbit, 1);
		uint32_t reg = 0u;
		if (bit)
		{
			if (processed.find(tex)==processed.end()) {
				processed.insert(tex);
				reg = regNum++;
			}
			else
				bit = 0;
		}
		i = core::bitfieldInsert(i, bit, dstbit, 1);
		i = core::bitfieldInsert<instr_t>(i, reg, reg_bitfield_shift, BITFIELDS_REG_WIDTH);

		if (bit)
			_tex2reg.insert({tex,reg});
	};

	traversal_t traversal;
		
	for (instr_t i : _t)
	{
		const E_OPCODE op = getOpcode(i);

		if (op==OP_NOOP || op==OP_INVALID || op==OP_SET_GEOM_NORMAL)
			continue;

		i = core::bitfieldInsert<instr_t>(i, 0, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_FETCH_TEX_COUNT);
			
		const uint32_t bsdf_ix = core::bitfieldExtract(i, BITFIELDS_BSDF_BUF_OFFSET_SHIFT, BITFIELDS_BSDF_BUF_OFFSET_WIDTH);
		const SBSDFUnion& bsdf_data = _bsdfData[bsdf_ix];

		switch (getOpcode(i))
		{
		case OP_DIFFUSE: _IRR_FALLTHROUGH;
		case OP_PLASTIC:
			write_fetch_flag(i, bsdf_data.param[0].tex, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_SHIFT_ALPHA_U_TEX, BITFIELDS_REG_0_SHIFT);
			write_fetch_flag(i, bsdf_data.param[1].tex, BITFIELDS_FETCH_TEX_1_SHIFT, BITFIELDS_SHIFT_REFL_TEX, BITFIELDS_REG_1_SHIFT);
			break;
		case OP_DIFFTRANS:
			write_fetch_flag(i, bsdf_data.difftrans.transmittance.tex, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_SHIFT_TRANS_TEX, BITFIELDS_REG_0_SHIFT);
			break;
		case OP_DIELECTRIC: _IRR_FALLTHROUGH;
		case OP_CONDUCTOR:
			write_fetch_flag(i, bsdf_data.param[0].tex, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_SHIFT_ALPHA_U_TEX, BITFIELDS_REG_0_SHIFT);
			write_fetch_flag(i, bsdf_data.param[1].tex, BITFIELDS_FETCH_TEX_1_SHIFT, BITFIELDS_SHIFT_ALPHA_V_TEX, BITFIELDS_REG_1_SHIFT);
			break;
		case OP_COATING:
			write_fetch_flag(i, bsdf_data.coating.alpha.tex, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_SHIFT_ALPHA_U_TEX, BITFIELDS_REG_0_SHIFT);
			write_fetch_flag(i, bsdf_data.coating.sigmaA.tex, BITFIELDS_FETCH_TEX_1_SHIFT, BITFIELDS_SHIFT_SIGMA_A_TEX, BITFIELDS_REG_1_SHIFT);
			break;
		case OP_BUMPMAP:
			i = core::bitfieldInsert<instr_t>(i, 1u, BITFIELDS_FETCH_TEX_0_SHIFT, 1);
			write_fetch_flag(i, bsdf_data.bumpmap.bumpmap, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_REG_0_SHIFT);
			break;
		case OP_BLEND:
			write_fetch_flag(i, bsdf_data.blend.weight.tex, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_SHIFT_WEIGHT_TEX, BITFIELDS_REG_0_SHIFT);
			break;
		}
		//opacity is common for all
		write_fetch_flag(i, bsdf_data.param[2].tex, BITFIELDS_FETCH_TEX_2_SHIFT, BITFIELDS_SHIFT_OPACITY_TEX, BITFIELDS_REG_2_SHIFT);

		//do not add instruction to stream if it doesnt need any texture or all needed textures are already being fetched by previous instructions
		if (!core::bitfieldExtract(i, BITFIELDS_FETCH_TEX_0_SHIFT, BITFIELDS_FETCH_TEX_COUNT))
			continue;

		traversal.push_back(i);
	}

	return traversal;
}

core::unordered_map<uint32_t, uint32_t> CMaterialCompilerGLSLBackendCommon::createBsdfDataIndexMapForPrefetchedTextures(instr_stream::SContext* _ctx, const instr_stream::traversal_t& _tex_prefetch_stream, const core::unordered_map<instr_stream::STextureData, uint32_t, instr_stream::STextureData::hash>& _tex2reg) const
{
	core::unordered_map<uint32_t, uint32_t> ix2ix;

	for (instr_t instr : _tex_prefetch_stream)
	{
		const auto bsdf_ix = core::bitfieldExtract(instr, BITFIELDS_BSDF_BUF_OFFSET_SHIFT, BITFIELDS_BSDF_BUF_OFFSET_WIDTH);
		{
			auto found = ix2ix.find(bsdf_ix);
			if (found != ix2ix.end())
				continue;
		}

		const SBSDFUnion& bsdf_data = _ctx->bsdfData[bsdf_ix];
		auto new_bsdf_data = bsdf_data;
		for (uint32_t i = 0u; i < SBSDFUnion::MAX_TEXTURES; ++i)
			if (core::bitfieldExtract(instr, tex_prefetch::BITFIELDS_FETCH_TEX_0_SHIFT + i, 1))
			{
				auto found = _tex2reg.find(bsdf_data.param[i].tex);
				if (found == _tex2reg.end())
					continue;

				new_bsdf_data.param[i].tex.prefetch_reg = found->second;
			}

		ix2ix.insert({ bsdf_ix, _ctx->bsdfData.size() });
		_ctx->bsdfData.push_back(new_bsdf_data);
	}

	return ix2ix;
}

auto CMaterialCompilerGLSLBackendCommon::compile(SContext* _ctx, IR* _ir, bool _computeGenChoiceStream) const -> result_t
{
	result_t res;
	for (const IR::INode* root : _ir->roots)
	{
		traversal_t rem_pdf_stream;
		{
			remainder_and_pdf::CTraversalGenerator gen(_ctx, _ir);
			rem_pdf_stream = gen.genTraversal(root);
		}
		traversal_t gen_choice_stream;
		if (_computeGenChoiceStream)
		{
			gen_choice::CTraversalGenerator gen(_ctx, _ir);
			gen_choice_stream = gen.genTraversal(root);
		}
		core::unordered_map<uint32_t, uint32_t> ix2ix;
		traversal_t tex_prefetch_stream;
		{
			core::unordered_map<STextureData, uint32_t, STextureData::hash> tex2reg;
			tex_prefetch_stream = tex_prefetch::genTraversal(rem_pdf_stream, _ctx->bsdfData, tex2reg);

			ix2ix = createBsdfDataIndexMapForPrefetchedTextures(_ctx, tex_prefetch_stream, tex2reg);
		}

		adjustBSDFDataIndices(rem_pdf_stream, ix2ix);
		adjustBSDFDataIndices(gen_choice_stream, ix2ix);

		traversal_t normal_precomp_stream;
		{
			normal_precomp_stream.reserve(std::count_if(rem_pdf_stream.begin(), rem_pdf_stream.end(), [](instr_t i) {return getOpcode(i)==OP_BUMPMAP;}));
			for (instr_t i : rem_pdf_stream)
				if (getOpcode(i)==OP_BUMPMAP)
					normal_precomp_stream.push_back(i);
		}

		result_t::instr_streams_t streams;
		streams.rem_and_pdf = {res.remainder_and_pdf.size(), rem_pdf_stream.size()};
		res.remainder_and_pdf.insert(res.remainder_and_pdf.end(), rem_pdf_stream.begin(), rem_pdf_stream.end());
		streams.gen_choice = {res.generator_choice.size(), gen_choice_stream.size()};
		res.generator_choice.insert(res.generator_choice.end(), gen_choice_stream.begin(), gen_choice_stream.end());
		streams.tex_prefetch = {res.texture_prefetch.size(), tex_prefetch_stream.size()};
		res.texture_prefetch.insert(res.texture_prefetch.end(), tex_prefetch_stream.begin(), tex_prefetch_stream.end());
		streams.norm_precomp = {res.normal_precomp.size(), normal_precomp_stream.size()};
		res.normal_precomp.insert(res.normal_precomp.end(), normal_precomp_stream.begin(), normal_precomp_stream.end());

		res.streams.push_back(streams);
	}

	return res;
}

}}}

