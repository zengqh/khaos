#pragma once
#include "KhaosRect.h"

namespace Khaos
{
	class LightNode;
	class Camera;
	class AxisAlignedBox;

	class LightManager : public AllocatedObject
	{
	public:
		static const int MAX_LIGHT_TYPES = 2;
		static const int MAX_BATCH_LITS = 32;

		struct LightItem : public AllocatedObject
		{
			LightItem() : lit(0) {}
			LightNode* lit;
			IntRect    aabbVP;
		};

		typedef vector<LightItem>::type ItemList;

	public:
		LightManager();
		~LightManager();

	public:
		void setCamera( Camera* cam );
		void addLight( LightNode* node );

		void beginPackLights();
		bool packLights();
		void endPackLights();

		bool hasDeferredLights() const;

		const LightItem* firstPackLight() const;
		int   getPackCount() const { return m_litPackCount; }

		const IntRect& getPackAABB() const { return m_curPackAABB; }
		const IntRect& getPackRangeIndex() const { return m_curRangeIndex; }

	private:
		void _calcAABBVP( const AxisAlignedBox& aabb, IntRect& ret ) const;

	private:
		Camera* m_currCamera;
		int m_currVPWidth;
		int m_currVPHeight;

		ItemList m_litList[MAX_LIGHT_TYPES];

		int m_litPackTypeIdx;
		int m_litPackFirstIdx;
		int m_litPackCount;

		IntRect m_curPackAABB;
		IntRect m_curRangeIndex;
	};
}

