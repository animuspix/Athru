#pragma once

#include <array>

namespace AthruGPU
{
	// Athru resource usage contexts; instances for GPU utilities, rendering, physics, and ecology
	// Used to offset resources into appropriate descriptor tables
	enum class RESRC_CTX
	{
		GENERIC,
		RENDER,
		PHYSICS,
		ECOLOGY
	};

	// Resource count per-context
	// No physics/ecology resources defined atm
	static constexpr u4Byte RESRCES_PER_CTX[4] = { 2, 22, 0, 0 };

	// Small convenience structure to wrap lambdas inside a returnable type
	template<typename callable>
	struct InitFn
	{
		void operator()() { fn(); };
		private:
			callable fn;
	};

	// Resource context representation/initializer
	template<typename...resrcInitLambdas>
	class ResrcContext
	{
		public:
			ResrcContext(std::tuple<resrcInitLambdas...> resrcInitters, const RESRC_CTX context, bool deferInit) :
			initters(resrcInitters), ctx(context)
			{
				if (!deferInit)
				{
					Init();
				}
			}
			template<typename...extraResrcInitLambdas>
			void Expand(std::tuple<extraResrcInitLambdas...> extraResrcInitters)
			{
				assert(!initted); // Expansion should not occur after initialization
				std::tuple_cat(initters, extraResrcInitters);
			}
			void Init()
			{
				if (!initted)
				{
					if (ctx == RESRC_CTX::GENERIC)
					{
						#define GENERIC_ORDER 1,0 // Two generic resources atm, initialized in reverse
						initCtx<GENERIC_ORDER>();
					}
					else if (ctx == RESRC_CTX::RENDER)
					{
						#define RENDER_ORDER 0, 3, 5, 6, 7, 8, 9, 20, 1, 10, 11, 12, 13, 14, 15, 16, 4, 2
						initCtx<RENDER_ORDER>();
					}
					else if (ctx == RESRC_CTX::PHYSICS)
					{
						// No defined physics resources yet...
						#ifdef PHYSICS_ORDER
						initCtx<bindingOrder...>();
						#endif
					}
					else if (ctx == RESRC_CTX::ECOLOGY)
					{
						// No defined ecology resources yet...
						#ifdef ECOLOGY_ORDER
						initCtx<ECOLOGY_ORDER>();
						#endif
					}
				}
			}
		private:
			std::tuple<resrcInitLambdas...> initters;
			template<typename...emptyBindSet>
			void initCtx() { initted = true; }
			template<int i, int...bindingOrder>
			void initCtx()
			{
				if (!initted)
				{
					typedef std::tuple_element<0, decltype(initters)>::type t;
					constexpr bool validInitter = std::is_invocable<t, void>::value;
					if constexpr (validInitter)
					{
						(std::get<i>(initters))();
					}
					initCtx<bindingOrder...>();
				}
			}
			bool initted = false;
			RESRC_CTX ctx;
	};
}