//////////////////////////////////////////////////////////////////////////////
//
// Geometry file for the source conector for the calibration sources
//
// The geometry consists of
//  1) Source connector
// All units are in mm.
//
//////////////////////////////////////////////////////////////////////////////

{
type: "GEO",
version: 1,
index: "world",
run_range: [0, 0],
pass: 0,
comment: "",
timestamp: "",
enable: 1,
//invisible: 1,

factory: "solid",
solid: "box",

mother: "",
}

{

index: "SourceConnector",
run_range: [0, 0],
enable: 1,
visible: 1,
pass: 0,
comment: "",
timestamp: "",

factory: "SourceConnector",
type: "GEO",
version: 1,
mother: "inner_av",
//for use with a plain box
//mother: "world",


// The centre of the source connector
sample_position: [0.0, 0.0, 0.0],
//quick connect
quick_connect_radius: 33.5,
quick_connect_inner_radius: 24.0,
quick_connect_height: 128.0,//(60-20)+48+(60-20)
quick_connect_plate_thickness: 10.0,
quick_connect_material: "stainless_steel",
quick_connect_colour:[0.7,0.7,0.7],//grey

//fill in gaps with air
air_material: "air"
air_colour: [0.0, 1.0, 1.0, 0.5],//cyan

// If you want to check for overlapping volumes when placing them (debugging)
check_overlaps: 1,
}
