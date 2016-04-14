#pragma once

class Box;
class Player;

class renderSegment {
	public:
		// Mixes the different fractals together to give
		// an overall terrain function; each [strength]
		// variable is used to derive the relative
		// weight of the different terrain types within
		// the average used in the overall function

		// Terrain types: hills, rivers, ocean, valleys, 
		// mountains, lakes, canyons, plateaux, plains

		void calculateTerrain(Player * PlayerInstance, long long seed);

		// Every type of terrain is a fractal, as described above;
		// the position of particular voxels is taken by
		// getting the player's position on a perfect sphere of
		// radius [averageRadius] then adding in their 
		// displacement away from the planet centre and 
		// "unwrapping" their position into a 2D rectangular
		// plane of dimensions 
		// (((2 * pi) * [averageRadius]) * ((2 * pi) * [averageRadius]))
		// this gives us the player's position on an flat plane
		// with equal area to the average area of the planet, and thus
		// the terrain within the current render segment (see render
		// explanation in [chunkHandler] for what this means) - just
		// substitute the player's transposed position into the
		// row and column of the 2-dimensional array holding the the
		// state of each point in the fractal plane (used = true, 
		// empty = false)

		// Write a DETAILED description of sound and geometry in the
		// repo's wiki section; this is rapidly getting too large to
		// fit in my head and I need a proper reference.

		// Summary:
		//
		// Planets are managed by a self-contained coordinate system
		// based on shells, discs, and segments
		//
		// Planetary positions are understood in terms of the most
		// massive object at the reference scale; in terms of the
		// universe, you'd use a large galactic cluster; in terms
		// of galaxies, you'd use a black hole; and in terms of 
		// star systems, you'd use the local star. This
		// flexibility references the real centrelessness of the
		// universe, and allows positional calculations to be kept
		// only as complex as they need to be - if planets were
		// positioned relative to the universal centre of mass
		// we'd need to account for orbital mechanics, universal
		// expansion, comets, asteroids, the effects of magnetic
		// dust, and all sorts of funky things like that, but this
		// way we just need to consider their orbital path around
		// the star (which we can assume to be circular), and the
		// small amount of displacement from planets around them.
		// Simples! For planetary events, we don't even need to
		// bother with that; just specify a shell, a disc, and a
		// segment, and you're golden.
		//
		// Planetary geometry is calculated in a multi-step
		// process: first, unwrap the current render segment into
		// a plane having area ([segmentRootArea] * [segmentRootArea])
		// (these are the [fractalPlane] objects you can see inside the
		// "Rendering and Procedural Generation" folder); second, apply
		// the appropriate equation to draw a fractal into this plane
		// (reality check: the [fractalPlane] objects don't actually
		// contain images; they contain 2-dimensional boolean arrays 
		// with the dimensions specified before, and just fill values
		// containing a point as 'true' and values without points as
		// 'empty'); third, repeat this process with assorted other
		// fractals, one for each feature you want to express; fourth,
		// combine the fractals together into a single plane containing
		// unique geometry (again, this is represented through a boolean
		// array) (the contributions from each individual type of terrain
		// can be moderated during this process, so that particular forms
		// are more or less visible in the terrain actually drawn to the
		// screen); fifth, go back over the actual terrain space while
		// you crawl the array within the output [fractalPlane] and check
		// if there's any values there; if there are, create a voxel, and
		// if not, leave that space at zero standard meters and fill it
		// with water up to the z-depth of the closest voxel group 
		// (z-values are discussed further down - the two best options are
		// to calculate them from point density, or to assign them to 
		// the complex values produced by complex IFS fractals) (what these
		// are and why they're relevant is /also/ discussed further down)
		// 
		// Fractals shouldn't specify terrain types, because every fractal
		// contains a diverse range of terrains simply by itself. However,
		// different materials DO tend to appear in different groups of
		// terrain geometries. So assign fractals to different materials
		// instead! This has the three-fold effect of (a) directly 
		// implementing materials within the game, (b) solving the 
		// fractals-don't-map-to-terrains dilemna, and (c) stopping the
		// material-distribution-within-fractals headache before it even
		// appearss
		//
		// Although I've written elsewhere about summing xy values and
		// then dividing them by the height of the segment to create fresh
		// z-values, this is actually a terrible idea - it would create a
		// linear wave in the terrain and prevent any sort of complex
		// geometry from arising, as top-right values would inevitably be
		// the highest ones in the plot. A more practical solution (at
		// least until higher dimensions can be /properly/ imitated
		// with complex numbers) is to take z-values through density;
		// every voxel is assigned a basic z-value of exactly 0 standard
		// meters, which increases by the number of other voxels within a
		// twenty-metre circle around it - if there are 100 voxels
		// arranged inside a twenty-metre circle with none beyond that,
		// the innermost voxel will be raised 100 places; each adjacent
		// voxel to that will increase 99 voxels, then 98, and so on and
		// so on until you have a roughly sinoidal wave segment; highest
		// in the centre, then lower, and rebounding at the edge. Where 
		// high and low density areas meet, the fringe z-values should be
		// averaged to produce an interpolated slope.
		// 
		// The major flaw with this approach is that ALL high-density 
		// regions are displaced - it's impossible to have high-density
		// areas within valleys or underwater. The best solution really is
		// to use complex fractals, but I feel that affline ones are much
		// better for alpha testing since they're so much less complicated
		// to implement
		// 
		// There is an issue with all of this - oceans. Oceans have a
		// a normalized surface, so describing them in terms of the 
		// z-values of individual voxels doesn't make any sense. 
		// Probably the smartest approach to this is to give water a
		// "null" fractal, one which does accumulate height but cuts off
		// at zero standard meters, and which holds no values at all - it
		// serves to poke holes in the output map, creating circular
		// little patches that then get filled with water in the render
		// process. 
		//
		// The weighting of different materials should be controlled by
		// altitude and context - if the previous 5 segments contained a
		// lot of non-water voxels (including the current one), the next
		// segment should weight water more heavily in it's output. If 
		// the player is a long way above or below the shell containing
		// the waterline, water should be heavily restricted regardless
		// and there should be lots of rock and sand but very little dirt,
		// grass, or life in general
		//
		// Planetary radius includes the outer limit of the atmosphere 
		// (~100km  on earth)
		//
		// The "weight" of water should follow an inverse parabola: very
		// low at the centre of the planet, very high at zero standard
		// meters, and logarithmically lower for every standard meter
		// afterward
		//
		// The boolean array should actually be an array of boxes; this
		// way each index can contain a material type (e.g., sand, 
		// water...) as well as whether that point exists in the current
		// segment or not
		// 
		// All fractals used should be different types of IFS fractal -
		// http://hiddendimension.com has a good explanation of these
		// IFS Fractals do have a complex form that produces much nicer,
		// more 3D-friendly shapes, but these require more computing
		// power and a greater understanding of mathematical logic to
		// solve. It might be best to start with simple affline 
		// (ordinary) IFS's, then move on to complex IFS's once we know
		// we can handle the more basic data
		//
		// Seeds and Player Position
		// Seeds are very large numbers, used to start procedural
		// algorithms such as pseudo-random number generators. Athru has
		// ten (that's an assumption - there may well be more materials 
		// than that) of them, with each one being an array of six
		// smaller arrays having (n) values where (n) is equal to the
		// number of transformations expected by a particular fractal; 
		// the values stored in each index of the smaller arrays are the
		// coefficients of each transformation (how much that particular
		// transformation influences a particular point), except for the
		// sixth index which holds the /probability/ that a particular
		// transformation will be used (transformations are applied 
		// randomly, although the selection MUST follow the frequencies
		// specified in the probability index (these can sum to less than 
		// 1, so randomness isn't necessarily sacrificed))
		// Seed weighting is highly mutable, but it isn't seeded; 
		// depending on distance from the planetary centre and a few other
		// factors, each weight is just a bounded pseudo-random number
		// 
		// Seeds are not affected by the position of the player; however,
		// some other things are. Player position affects the current
		// coordinate system (planet-level (shells/discs/segments), 
		// system-level (orbital speed, orbital position, orbital 
		// characteristics in relation to the star and nearby planets),
		// galactic (system-level, with the largest attractor being a 
		// black hole), or universal (galactic, though the largest 
		// attractor is a cluster of galaxies)) (coordinate systems are
		// decided by the most massive nearby object - if you are on a 
		// a planet, planetary coordinates will be in effect; if you're 
		// within ten AU of a star, system level coordinates will set in,
		// galactic coordinates go into effect when you're close enough
		// to a galaxy to see it but almost equally far from any 
		// particular star within, and universal coordinates become apparent
		// when you are equally distant from any particular galaxy), and it
		// also affects which materials the player is likely to see 
		// (different shells/discs/segments are associated with different 
		// weightings for each fractal); mainly, though, its crucial for
		// loading such a massive world into RAM without destroying the
		// computer. If we know that the player is in the first shell,
		// fourth disc, second segment (since each is just 10M deep they'd
		// be suffocating at that altitude, but whatevs), we can safely
		// generate an example of /what the terrain would look like/ within
		// that region of the planet without knowing anything else - all we
		// need to do is plug in your position, weight each fractal
		// with regards to how materials are distributed in that region, 
		// merge them into a single output fractal, then draw that fractal
		// into the current segment. Without the player's position, it would
		// be impossible to know what terrain to draw and what rules to
		// follow in the process, so you'd end up with a much broader set of
		// rules and entire planets being rendered simultaneously.
		//
		// Player position on/within the planet is fairly easy - they exist
		// inside a shell, a disc, and a segment. Extra-planetary player
		// position is much more complicated, and involves pages and pages of
		// fudging around with orbital mechanics, literal astrophysics, and
		// other demons. It's important, but we should keep our eyes on
		// single-planet simulations before we run off and start fiddling
		// around with space travel (you'll note the gist of extra-planetary
		// coordinates is described above - reference everything relative to
		// the largest mass, store the characteristics of the current orbit 
		// and the bodies most likely to affect it, and store where in that
		// orbit the player is; the same logic applies regardless of whether
		// you're working at system-level, galactic, or universal scale,
		// just with more and more masses involved and thus with longer 
		// calculations, so it should be fine until we actually start to 
		// implement space travel and refine it a bit)
		//
		// It should be noted that, despite my liberal use of the word "draw"
		// and it's declensions above, only a fraction of a segment (or 
		// perhaps multiple segments - it still isn't clear how we'll handle
		// sight that passes through multiple segments) (and it will - each
		// segment is only 200 standard meters wide)) is actually rendered
		// out; the rest is stored in the output [fractalPlane]'s array of
		// [Box]es, to be properly rendered out when it passes into the
		// camera's view cone.
		//
		// We need to figure out how to optimise this for multi-segment
		// viewing - looking out from mountain-tops, through the window of
		// a descending spaceship, things like that. LODding could work, but
		// generating kilometers of terrain on-the-fly would still be 
		// EXTREMELY cpu/gpu-intensive
		//
		// The background music gains pitch as the z-tendency 
		// increases, tempo as the x-tendency increases, and frequency 
		// as the y-tendency increases; the inverse is also applicable
		// Eventually, one of us will compose the background music
		// itself, and it will create a gentle, ambient sonarium in the
		// same style as C418's music for Minecraft
		//
		// Unknown/to discuss: how to handle light + how to handle complex
		// entities e.g. animals, spaceships, people; also what sort of
		// view-cone to use and how to convert from 3D raw output to a 
		// displayable 2D projection (essentially, how to force all this
		// fancy data into a camera so the player has something to look 
		// at)

	private:
		unsigned char renderSegmentArc;
		unsigned char renderSegmentWidth;
		unsigned char renderSegmentRadius;
};