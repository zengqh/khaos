#include "KhaosPreHeaders.h"
#include "KhaosAxisAlignedBox.h"

namespace Khaos
{
	const AxisAlignedBox AxisAlignedBox::BOX_NULL;
    const AxisAlignedBox AxisAlignedBox::BOX_UNIT( Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f) );
	const AxisAlignedBox AxisAlignedBox::BOX_INFINITE(AxisAlignedBox::EXTENT_INFINITE);
}

