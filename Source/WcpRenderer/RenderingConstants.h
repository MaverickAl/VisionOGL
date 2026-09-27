#pragma once

enum ProjectionMode
{
	PM_PERSPECTIVE = 0, // Render using perspective projection
	PM_ORTHO,			// Render using orthogonal projection
	PM_2D,				// Render directly to screen co-ordinates - for menus etc
	PM_2D_WORLD,		//Render directly to screen co-ordinates, accounting for the full screen hack if active
	PM_NONE
};
