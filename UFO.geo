//////////////////////////////////////////////////////////////////////////////
//
// Geometry file for the UFO for the calibration sources
//
// The geometry consists of
//  1) Representation of the electronics
//  2) Acrylic container
//  3) o-ring
//  4) Top cap
//  5) Bottom Cup
//  6) Disc	
//  7) Air
//
// All units are as in the blue prints.
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

index: "UFO",
run_range: [0, 0],
enable: 1,
visible: 1,
pass: 0,
comment: "",
timestamp: "",

factory: "UFO",
type: "GEO",
version: 1,
mother: "inner_av",
//for use with a plain box
//mother: "world",


// The centre of the UFO
sample_position: [0.0, 0.0, 0.0],
//electronics mode as a disc mm
electronics_rad: 25.0,
electronics_thk: 0.5,
electronics_material:"acrylic_sno",
electronics_colour: [1.0, 1.0, 0.0, 0.5], // yellow

// If you want to check for overlapping volumes when placing them (debugging)
check_overlaps: 1,

// Acrylic parameters mm
acrylic_radius: 31.75,//outer dimension
acrylic_height: 45.0,//main height
acrylic_inner_rad: 25.4,
acrylic_collar_height: 7.,
acrylic_collar_rad: 30.25,
acrylic_oring_groove_thickness: 2.37,
acrylic_oring_groove_height: 0.33,
acrylic_oring_groove_rad: 28.85,
acrylic_LED_height: 15.0,
acrylic_LED_rad: 1.5,
acrylic_LED_depth: 30.0,

acrylic_material: "acrylic_sno",
acrylic_colour: [0.0, 0.0, 1.0, 0.5], // blue

//oring 
oring_material: "G4_VITON",
oring_colour: [1.0, 0.0, 0.0, 0.5],//red

//UFO cap inch
cap_radius: 1.25,
cap_inner_rad: 0.315,
cap_space_rad: 1.195,
cap_space_thk: 0.23,
cap_thickness: 0.73,
cap_material: "stainless_steel",
cap_colour: [0.5, 0.5, 0.5, 0.5], // gray

//bottom cup inch
bottom_cup_radius: 1.25,
bottom_cup_height: 2.00,
bottom_cup_top_inner_rad: 1.195,
bottom_cup_top_height: 0.478,
bottom_cup_mid_inner_rad: 0.985,
bottom_cup_mid_height: 0.636
bottom_cup_bot_inner_rad: 0.885,
bottom_cup_bot_outer_rad: 1.005,
bottom_cup_bot_outer_height: 0.69,
bottom_cup_mid_top_height: 1.23,
bottom_cup_gap: 0.05,
bottom_cup_material: "stainless_steel",
bottom_cup_colour: [0.6, 0.6, 0.6, 0.5], // gray

//bottom disc mm
bottom_disc_radius: 30.25,
bottom_disc_inner_radius: 10.,
bottom_disc_thickness: 6.35,
bottom_disc_hole_radius: 2.25,
bottom_disc_distance_rad: 19,
bottom_disc_material: "stainless_steel",
bottom_disc_colour: [0.4, 0.4, 0.4, 0.5], // gray

//fill in gaps with air
air_material: "air"
air_colour: [0.0, 1.0, 1.0, 0.5],//cyan

}
