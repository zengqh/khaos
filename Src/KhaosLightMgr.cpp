#include "KhaosPreHeaders.h"
#include "KhaosLightMgr.h"
#include "KhaosLightNode.h"
#include "KhaosCamera.h"

namespace Khaos
{
	LightManager::LightManager() :
		m_currCamera(0), m_currVPWidth(0), m_currVPHeight(0),
		m_litPackTypeIdx(0), m_litPackFirstIdx(0), m_litPackCount(0)
	{
	}

	LightManager::~LightManager()
	{

	}

	void LightManager::setCamera( Camera* cam )
	{
		m_currCamera   = cam;
		m_currVPWidth  = cam->getViewportWidth();
		m_currVPHeight = cam->getViewportHeight();
	}

	void LightManager::addLight( LightNode* node )
	{
		LightItem* item = 0;

		switch ( node->getLightType() )
		{
		case LT_POINT:
			m_litList[0].push_back( LightItem() );
			item = &m_litList[0].back();
			break;

		case LT_SPOT:
			m_litList[1].push_back( LightItem() );
			item = &m_litList[1].back();
			break;

		default:
			khaosAssert(0);
			return;
		}

		item->lit = node;
		_calcAABBVP( node->getWorldAABB(), item->aabbVP );
	}

	void LightManager::_calcAABBVP( const AxisAlignedBox& aabb, IntRect& ret ) const
	{
		Vector3 minPt = m_currCamera->getViewProjMatrix() * aabb.getMinimum();
		Vector3 maxPt = m_currCamera->getViewProjMatrix() * aabb.getMaximum();

		float min_x = minPt.x * 0.5f + 0.5f;
		float max_x = maxPt.x * 0.5f + 0.5f;
		float min_y = minPt.y * -0.5f + 0.5f;
		float max_y = maxPt.y * -0.5f + 0.5f;

		ret.left   = (int)(min_x * m_currVPWidth);
		ret.right  = (int)(max_x * m_currVPWidth) + 1; // 不用很精确,+1总是取上整
		ret.top    = (int)(min_y * m_currVPHeight);
		ret.bottom = (int)(max_y * m_currVPHeight) + 1;
	}

	bool LightManager::hasDeferredLights() const
	{
		return m_litList[0].size() || m_litList[1].size();
	}

	void LightManager::beginPackLights()
	{
		m_litPackTypeIdx = 0;
		m_litPackFirstIdx = 0;
		m_litPackCount = 0;
	}

	void LightManager::endPackLights()
	{
	}

	const LightManager::LightItem* LightManager::firstPackLight() const
	{
		return &m_litList[m_litPackTypeIdx][m_litPackFirstIdx];
	}

	bool LightManager::packLights()
	{
		if ( m_litPackTypeIdx >= MAX_LIGHT_TYPES )
			return false;

		// 当前打包的组
		ItemList& curLights = m_litList[m_litPackTypeIdx];

		// 从上一次结束位置开始
		m_litPackFirstIdx = m_litPackFirstIdx + m_litPackCount;

		// 检查是否已经本组打包完成
		if ( m_litPackFirstIdx >= (int)curLights.size() ) 
		{
			khaosAssert( m_litPackFirstIdx == curLights.size() );

			// 到下一个组新开始
			++m_litPackTypeIdx;
			m_litPackFirstIdx = 0;
			m_litPackCount = 0;

			return packLights();
		}

		// 开始本组打包
		m_litPackCount = MAX_BATCH_LITS;
		if ( m_litPackFirstIdx + m_litPackCount > (int)curLights.size() ) // 最后一个组可能超过
			m_litPackCount =  (int)curLights.size() - m_litPackFirstIdx;

		m_curPackAABB.setEmpty();

		int endIdx = m_litPackFirstIdx + m_litPackCount;

		for ( int id = m_litPackFirstIdx; id < endIdx; ++id )
		{
			const LightItem& item = curLights[id];
			m_curPackAABB.merge( item.aabbVP ); // 计算合并aabb
		}

		m_curPackAABB.intersect( IntRect(0, 0, m_currVPWidth, m_currVPHeight) ); // 最终结果限制在屏幕内

		// 在tile中范围
		m_curRangeIndex.left   = m_curPackAABB.left / TILE_GRID_SIZE;
		m_curRangeIndex.top    = m_curPackAABB.top / TILE_GRID_SIZE;
		m_curRangeIndex.right  = m_curPackAABB.right / TILE_GRID_SIZE;
		m_curRangeIndex.bottom = m_curPackAABB.bottom / TILE_GRID_SIZE;

		return true;
	}
}

