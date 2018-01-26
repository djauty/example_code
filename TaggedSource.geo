//////////////////////////////////////////////////////////////////////////////
//
// Geometry file for the Cobalt-60 calibration source
//
// The geometry consists of
//  1) A container with flange
//  2) A stem with flange that attaches to the container
//  3) Screws and nuts to hold the two together
//  3) A PMT inside the container
//  4) A plastic scintillator button attached to the PMT face and 
//     which is a sensitive detector
//  5) Potting compound that fills part of the inner container around the PMT
//
// The position of the source is defined by the position of the defined isotope in the 
// source using the variable "sample_position"
//
// The activity of the source in Bq is given by "ref_activity" with reference
// date given by "ref_date"
//
// The screws/nuts can be enabled using "screws_enable"
//
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

material: "pmt_vacuum",
half_size: [1000.0,1000.0,1000.0],
vis_invisible: 0,
vis_style: "wireframe",
vis_color: [0.67, 0.29, 0.0],
}


{

index: "TaggedSource",
run_range: [0, 0],
enable: 1,
visible: 1,
pass: 0,
comment: "",
timestamp: "",

factory: "TaggedSource",
type: "GEO",
version: 1,
mother: "inner_av",
//for use with a plain box
//mother: "world",

// The centre of the scintillator button, where the radio isotope resides
sample_position: [0.0, 0.0, 0.0],

// The activity (and error) of the source on a reference date (in Bq)
// This is for Co60 to get accurate info for other sources add in .mac file
ref_activity: 200.,
ref_activity_err: 1.,
ref_date: "18 Sep 2014 12:00:00",

// If you want to check for overlapping volumes when placing them (debugging)
check_overlaps: 1,

// Container parameters
container_radius: 23.7,//outer dimension
container_height: 58.0,//main height
container_thickness: 2.0,
container_upper_height: 3.,
container_inner_lip_thickness: 1.0,
container_collar_height: 10.,
container_collar_hole_width: 25.,
container_collar_hole_rad: 14.,
container_slope_height:6.17,
container_flange_radius: 34.39,
container_flange_thickness: 9.0,
container_flange_base_height: 1.5,
container_screw_hole_radius: 1.45,
container_nut_groove_height: 4.0,
container_nut_groove_width: 7.35,
container_gap_copper: 0.5,
container_material: "G4_POLYOXYMETHYLENE",
container_colour: [0.0, 0.0, 0.0, 0.5] ,  // black

// Stem parameters
stem_flange_thickness: 5.0,
stem_screw_head_hole_radius: 2.35,
stem_screw_head_hole_depth: 2.90,
connect_radius: 34.39,//check
connect_thickness: 9.53,
bore_radius: 5.0,
stem_flange_end_radius: 11.0,
stem_flange_end_length: 8.7376,
stem_connect_end_radius: 15.88,
stem_length: 309.74,
stem_taper_angle: 3.0,
stem_material: "G4_POLYOXYMETHYLENE",
stem_colour: [0.0, 0.0, 0.0, 0.5] ,  // black

// O-ring parameters
oring_groove_height: 1.27,
oring_groove_width: 2.26,
oring_groove_inner_radius: 25.26,
oring_material: "G4_VITON",
oring_colour: [1.0, 0.0, 0.0, 0.5],

// Screw and nut parameters
screws_enable: 1,
number_of_screws: 8,
screw_distance_from_centre: 30.715,//check
screw_head_radius: 2.27,
screw_radius: 1.415,
screw_head_length: 2.8,
screw_length: 15.0,//check
screw_material: "stainless_steel",
screw_colour: [0.5, 0.5, 0.5, 0.5], // gray
nut_radius: 3.545,
nut_insert_thickness: 1.0,
nut_thickness: 3.78,
nut_material: "stainless_steel",
nut_insert_material: "nylon",
nut_colour: [0.5, 0.5, 0.5, .5], // gray
nut_insert_colour: [1.0, 1.0, 1.0, 0.5], // white

//Collar screw and nut parameters
number_of_screws_collar: 4,
number_of_screws_copper: 8, //copper only
screw_distance_from_centre_collar: 23.49,//check
screw_head_radius_collar: 2.27,//check
screw_radius_collar: 1.415,//check
screw_head_length_collar: 2.8,//check
screw_length_collar: 12.0,//check
screw_length_copper: 12.0,//check
nut_radius_collar: 3.545,//check
nut_insert_thickness_collar: 1.0,//check
nut_thickness_collar: 3.78,//check



// PMT parameters
pmt_window_radius: 5.0,
pmt_active_radius: 4.0,
pmt_window_inset: 1.5,
pmt_length: 50.0,
pmt_face_length: 22.0,
pmt_face_length: .5,
pmt_material: "aluminum",
pmt_active_material: "glass",
pmt_colour: [0.0, 0.0, 1.0, 0.5], // blue

// Scintillator button parameters
scintillator_radius: 4.0,
scintillator_thickness: 4.0,
scintillator_material: "G4_PLASTIC_SC_VINYLTOLUENE",
scintillator_colour: [0.0, 0.5, 0.5, 0.5], 

// Potting compound parameters
potting_depth: 20.0,
potting_material: "acrylic_sno",//PLACEHOLDER, REALLY BISPHENOLA/EPICHLOROHYDRIN
potting_colour: [1.0, 1.0, 0.0, 0.5], // yellow

// Sensitive detector parameters
sensitive_detector: "/mydet/pmt/calib",
//sensitive_detector: "button",
lcn: 9188,   // FECD channel 4, card 15, crate 17
source_efficiency: 0.9,
energy_threshold: 0.0,

//copper container
copper_gap: 0.5,
copper_height: 67.5,
copper_width: 23.75,
copper_thickness: 0.0127,
copper_flange_rad: 21.615,
copper_flange_height:2.5,
copper_flange_lip_height: 5,
copper_flange_lip_width: 23.49,
copper_flange_lip_thickness: 1,
copper_glass_rad: 6.65,
copper_metal_rad: 7.25,
copper_glass_height: 3,
copper_oring_inner_rad: 15.43,
copper_oring_outter_rad: 15.83,
indium_depth_bottom: 0.75,
indium_depth_top: 0.5,

copper_material: "G4_Cu",
copper_colour: [1.0, 0.0, 1.0, 0.5],//magenta
indium_material: "G4_In",
indium_colour: [1.0, 1.0, 1.0, 0.5],//white
glass_material: "glass",
glass_colour:[0.0, 0.5, 1.0, 0.5],

//fill in gaps with air
air_copper_removal: 23.26,
air_material: "air"
air_colour: [0.0, 1.0, 1.0, 0.5],//cyan
}
