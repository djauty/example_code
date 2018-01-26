////////////////////////////////////////////////////////////////////////
// Last svn revision: $Id$
////////////////////////////////////////////////////////////////////////

#include <RAT/GeoTaggedSourceFactory.hh>

#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <RAT/Detector.hh>
#include <RAT/Materials.hh>
#include <RAT/string_utilities.hpp>
#include <RAT/EnvelopeConstructor.hh>
#include <RAT/PMTConstructorParams.hh>
#include <RAT/CalibPMTSD.hh>
#include <RAT/ChannelEfficiency.hh>

#include <G4Material.hh>

#include <G4ThreeVector.hh>
#include <G4RotationMatrix.hh>

#include <G4Tubs.hh>
#include <G4Cons.hh>
#include <G4Box.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>
#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>
#include <G4SDManager.hh>

#include <G4VisAttributes.hh>
#include <G4Color.hh>

#include <vector>
#include <string>
#include <algorithm>

namespace RAT
{
  void GeoTaggedSourceFactory::SetColor(DBLinkPtr table, G4String colorName, G4LogicalVolume *logicalVolume)
  {
    // Set the color of a logical volume

    G4VisAttributes *vis = new G4VisAttributes();
    try {
      const std::vector<double> &color = table->GetDArray(colorName);
      Log::Assert(color.size() == 3 || color.size() == 4, "GeoTaggedSourceFactory: Color entry " + colorName + " does not have 3 (RGB) or 4 (RGBA) components");
      if(color.size() == 3) // RGB
        vis->SetColour(G4Colour(color[0], color[1], color[2]));
      else if(color.size() == 4) // RGBA
        vis->SetColour(G4Colour(color[0], color[1], color[2], color[3]));
    }
    catch(DBNotFoundError &e){
      Log::Die("GeoTaggedSourceFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field +".");
    };

    logicalVolume->SetVisAttributes(vis);
  } // SetColour

  std::vector<double> GeoTaggedSourceFactory::MultiplyVectorByUnit(std::vector<double> v, const double unit) {
    transform(v.begin(),v.end(),v.begin(),bind2nd(std::multiplies<double>(),unit));
    return v;
  } // MultiplyVectorByUnit

  G4PVPlacement* GeoTaggedSourceFactory::G4PVPlacementWithCheck(
                                                                G4Transform3D& Transform3D,
                                                                G4LogicalVolume* pCurrentLogical,
                                                                const G4String& pName,
                                                                G4LogicalVolume* pMotherLogical,
                                                                G4bool pMany,
                                                                G4int pCopyNo,
                                                                G4bool pSurfChk)
  {
    G4PVPlacement* placement = new G4PVPlacement(Transform3D, pCurrentLogical, pName, pMotherLogical, pMany, pCopyNo, false);
    Log::Assert(!pSurfChk || !placement->CheckOverlaps(),
                "GeoTaggedSourceFactory: Overlap detected when placing volume " +
                pName + ". See log for details.");
    return placement;
  } // G4PVPlacementWithCheck


  void GeoTaggedSourceFactory::Construct(DBLinkPtr table,
                                         const bool checkOverlaps)
  {

    // Define a default rotation matrix that does not rotate, and other constants
    G4RotationMatrix *noRotation = new G4RotationMatrix();
    const bool pMany = false;
    const int pCopyNo = 0;

    // Get a link to the database storing the source PMT geometry information
    DBLinkPtr pmtTable = DB::Get()->GetLink("PMT","Co60PMT");

    try { // To catch DBNotFoundError
      // Check for overlap when placing volumes?
      const bool pSurfChk = table->GetI("check_overlaps");

      const std::string index = table->GetIndex(); //Use table index as prefix
      const std::string prefix = index + "_";      // for volume names

      // Get the mother volume name and ensure it exists
      const std::string motherName = table->GetS("mother");
      //  G4LogicalVolume* const motherLog = FindMother(motherName);
      G4LogicalVolume * const motherLog = Detector::FindLogicalVolume(motherName);
      G4Material* motherMaterial = motherLog->GetMaterial();
      Log::Assert(motherLog != NULL,
                  "GeoTaggedSourceFactory: Unable to find mother volume '" +
                  motherName + "' for '" + index + "'.");

      // Get the position of the 60Co source and build the volume relative to it
      std::vector<double> const &pos =
        MultiplyVectorByUnit(table->GetDArray("sample_position"),CLHEP::mm);
      Log::Assert(pos.size() == 3,"GeoTaggedSourceFactory: sample_position does not have three components.");
      G4ThreeVector samplePosition(pos[0], pos[1], pos[2]);

      // =====================================
      // Read all parameters from the database
      // =====================================

      // Container parameters
      const double containerRadius = table->GetD("container_radius") * CLHEP::mm;
      const double containerHeight = table->GetD("container_height") * CLHEP::mm;
      const double containerThickness = table->GetD("container_thickness")*CLHEP::mm;
      const double containerCollarHeight =
        table->GetD("container_collar_height") * CLHEP::mm;
      const double containerCollarHoleWidth =
        table->GetD("container_collar_hole_width") * CLHEP::mm;
      const double containerCollarHoleRad =
        table->GetD("container_collar_hole_rad") * CLHEP::mm;
      const double containerUpperHeight =
        table->GetD("container_upper_height") * CLHEP::mm;
      const double containerSlopeHeight =
        table->GetD("container_slope_height") * CLHEP::mm;
      const double containerFlangeRadius =
        table->GetD("container_flange_radius") * CLHEP::mm;
      const double containerFlangeHeight =
        table->GetD("container_flange_thickness") * CLHEP::mm;
      const double containerFlangeBaseHeight =
        table->GetD("container_flange_base_height") * CLHEP::mm;
      const double containerScrewHoleRadius =
        table->GetD("container_screw_hole_radius") * CLHEP::mm;
      const double containerOringGrooveDepth =
        table->GetD("oring_groove_height") * CLHEP::mm;
      const double containerOringGrooveWidth =
        table->GetD("oring_groove_width") * CLHEP::mm;
      const double containerOringGrooveInnerRadius =
        table->GetD("oring_groove_inner_radius") * CLHEP::mm;
      const double containerNutGrooveHeight =
        table->GetD("container_nut_groove_height") * CLHEP::mm;
      const double containerNutGrooveWidth =
        table->GetD("container_nut_groove_width") * CLHEP::mm;
      G4Material* const containerMaterial =
        G4Material::GetMaterial(table->GetS("container_material"));

      //copper box
      const double copperBoxGap =
        table->GetD("copper_gap") * CLHEP::mm;
      const double copperBoxHeight =
        table->GetD("copper_height") * CLHEP::mm;
      const double copperBoxWidth =
        table->GetD("copper_width") * CLHEP::mm;
      const double copperBoxThickness =
        table->GetD("copper_thickness") * CLHEP::mm;
      const double copperBoxFlangeRadius =
        table->GetD("copper_flange_rad") * CLHEP::mm;
      const double copperBoxFlangeHeight =
        table->GetD("copper_flange_height") * CLHEP::mm;
      const double copperBoxFlangeLipHeight =
        table->GetD("copper_flange_lip_height") * CLHEP::mm;
      const double copperBoxFlangeLipWidth =
        table->GetD("copper_flange_lip_width") * CLHEP::mm;
      const double copperBoxFlangeLipThickness =
        table->GetD("copper_flange_lip_thickness") * CLHEP::mm;
      const double copperBoxGlassRadius =
        table->GetD("copper_glass_rad") * CLHEP::mm;
      const double copperBoxGlassHeight =
        table->GetD("copper_glass_height") * CLHEP::mm;
      const double copperBoxMetalRadius =
        table->GetD("copper_metal_rad") * CLHEP::mm;
      const double copperBoxOringInnerRad =
        table->GetD("copper_oring_inner_rad") * CLHEP::mm;
      const double copperBoxOringOuterRad =
        table->GetD("copper_oring_outter_rad") * CLHEP::mm;
      const double indiumDepthBottom =
        table->GetD("indium_depth_bottom") * CLHEP::mm;
      const double indiumDepthTop =
        table->GetD("indium_depth_top") * CLHEP::mm;
      G4Material* const copperMaterial =
        G4Material::GetMaterial(table->GetS("copper_material"));
      G4Material* const indiumMaterial =
        G4Material::GetMaterial(table->GetS("indium_material"));
      G4Material* const glassMaterial =
        G4Material::GetMaterial(table->GetS("glass_material"));

      // Stem parameters
      const double stemFlangeRadius = containerFlangeRadius;
      const double stemFlangeThickness =
        table->GetD("stem_flange_thickness") * CLHEP::mm;
      const double stemScrewHoleRadius = containerScrewHoleRadius;
      const double connectorRadius = table->GetD("connect_radius") * CLHEP::mm;
      const double connectorThickness = table->GetD("connect_thickness") * CLHEP::mm;
      const double boreRadius = table->GetD("bore_radius") * CLHEP::mm;
      const double stemFlangeEndRadius =
        table->GetD("stem_flange_end_radius") * CLHEP::mm;
      const double stemFlangeEndLength =
        table->GetD("stem_flange_end_length") * CLHEP::mm;
      const double stemConnectorEndRadius =
        table->GetD("stem_connect_end_radius") * CLHEP::mm;
      const double stemLength = table->GetD("stem_length") * CLHEP::mm;
      const double stemAngle = table->GetD("stem_taper_angle") * CLHEP::deg;
      const double stemAngledLength =
        fabs(stemConnectorEndRadius-stemFlangeEndRadius)/tan(stemAngle);
      const double stemConnectorEndLength = stemLength-stemFlangeThickness-
        connectorThickness-stemFlangeEndLength-stemAngledLength;
      G4Material* const stemMaterial =
        G4Material::GetMaterial(table->GetS("stem_material"));

      // O-ring parameters (o-ring will completely fill o-ring groove)
      G4Material* const oringMaterial =
        G4Material::GetMaterial(table->GetS("oring_material"));

      // Screw and nut parameters (allow for lock nuts, with a different
      // material insert)
      const bool screwsEnable = table->GetI("screws_enable");
      const int nScrews = table->GetI("number_of_screws");
      const double screwDistanceFromCentre =
        table->GetD("screw_distance_from_centre") * CLHEP::mm;
      const double screwHeadRadius = table->GetD("screw_head_radius") * CLHEP::mm;
      const double screwRadius = table->GetD("screw_radius") * CLHEP::mm;
      const double screwHeadLength = table->GetD("screw_head_length") * CLHEP::mm;
      const double screwLength = table->GetD("screw_length") * CLHEP::mm;
      const double nutRadius = table->GetD("nut_radius") * CLHEP::mm;
      const double nutInsertThickness =table->GetD("nut_insert_thickness")*CLHEP::mm;
      const double nutThickness = table->GetD("nut_thickness") * CLHEP::mm;

      G4Material* const screwMaterial =
        G4Material::GetMaterial(table->GetS("screw_material"));

      G4Material* const nutMaterial =
                     G4Material::GetMaterial(table->GetS("nut_material"));
      G4Material* const nutInsertMaterial =
        G4Material::GetMaterial(table->GetS("nut_insert_material"));

      // PMT parameters (H10721P-110) for both the active PMT part (the glass
      // cylinder) and the aluminum body
      const double pmtWindowRadius = table->GetD("pmt_window_radius") * CLHEP::mm;
      const double pmtActiveRadius = table->GetD("pmt_active_radius") * CLHEP::mm;
      const double pmtWindowInset = table->GetD("pmt_window_inset") * CLHEP::mm;
      const double pmtLength = table->GetD("pmt_length") * CLHEP::mm;
      const double pmtFaceLength = table->GetD("pmt_face_length") * CLHEP::mm;
      const double pmtFaceThickness = 0.1 * CLHEP::mm;
      G4Material* const pmtMaterial =
        G4Material::GetMaterial(table->GetS("pmt_material"));
      G4Material* const pmtActiveMaterial =
        G4Material::GetMaterial(table->GetS("pmt_active_material"));
      // PMT parameters for the active part of the PMT, which fires depending
      // on energy deposition in the scintillator volume and an efficiency
      // Ensure that the lcn for the pmt is one of the FECD channels, and not
      // that channel (9207) which reads the raw trigger signal
      const int lcn = table->GetI("lcn");
      Log::Assert(lcn >= 9184 && lcn <=9215 && lcn != 9207, "GeoTaggedSourceFactory: LCN "
                  +to_string(lcn)+" is outside the valid range of [9184,9215], excluding 9207.");
      std::string detectorName = table->GetS("sensitive_detector");
      const double pmtEfficiency = table->GetD("source_efficiency");
      const double pmtEnergyThreshold = table->GetD("energy_threshold") * CLHEP::MeV;

      // Scintillator button parameters
      const double scintRadius = table->GetD("scintillator_radius") * CLHEP::mm;
      const double scintThickness = table->GetD("scintillator_thickness")*CLHEP::mm;
      G4Material* const scintMaterial =
        G4Material::GetMaterial(table->GetS("scintillator_material"));

      //not sure how to make this from other mesurements
      const double airCopperRemoval = table->GetD("air_copper_removal") * CLHEP::mm;
      G4Material* const airMaterial =
      G4Material::GetMaterial(table->GetS("air_material"));


      // ===================================
      // Build the solid and logical volumes
      // and place them physically
      // ===================================



      // Place the container so that the source position remains as specified
      // relative to snoplus
      const double containerOffset = -containerThickness/2.-containerHeight-containerCollarHeight+copperBoxHeight-copperBoxGap-scintThickness/2.+(stemLength+containerFlangeHeight+containerSlopeHeight+containerUpperHeight+containerCollarHeight+containerHeight+containerThickness)/2.;



      // Container
      // Make the container out of a series of additions and subtractions,
      // since it is a cylinder with varying inner/outer radii
      // Begin with all of the pieces that will make the final volume
      G4VSolid* containerSolid1 = new G4Tubs(prefix+"container_solid1",0.0,
                                             containerRadius,containerThickness/2.0,0.0,CLHEP::twopi);//container base
      G4VSolid* containerSolid2 = new G4Tubs(prefix+"container_solid2",
                                             containerRadius-containerThickness,
                                             containerRadius,containerHeight/2.0,0.0,CLHEP::twopi);//container walls
      G4VSolid* containerSolid3 = new G4Tubs(prefix+"container_solid3",
                                             containerCollarHoleRad,containerRadius,
                                             containerCollarHeight/2.0,0.0,CLHEP::twopi);//container collar
      G4VSolid* containerSolid4 = new G4Box(prefix+"container_solid4",
                                            containerCollarHoleWidth/2.,containerCollarHoleWidth/2.,
                                            (containerCollarHeight+1)/2.);//square hole in collar
      G4VSolid* containerSolid5 = new G4Tubs(prefix+"container_solid5",
                                             containerRadius-containerThickness,
                                             containerRadius,containerUpperHeight/2.,0.,CLHEP::twopi);//Upper container before slope
      G4VSolid* containerSolid6 = new G4Cons(prefix+"container_solid6",
                                             containerRadius-containerThickness,containerRadius,
                                             containerRadius-containerThickness,containerFlangeRadius,
                                             containerSlopeHeight/2.,0.,CLHEP::twopi);//slope to flange Rad
      G4VSolid* containerSolid7 = new G4Tubs(prefix+"container_solid7",
                                             containerRadius-containerThickness,containerFlangeRadius,
                                             containerFlangeHeight/2.,0.,CLHEP::twopi);//Flange delrin
      G4VSolid* containerSolid8 = new G4Tubs(prefix+"container_solid8",
                                             containerFlangeRadius-containerNutGrooveWidth,containerFlangeRadius+1,
                                             containerNutGrooveHeight/2.,0.,CLHEP::twopi);//Flange groove
      G4VSolid* containerSolid9 = new G4Tubs(prefix+"container_solid9",
                                             containerOringGrooveInnerRadius,
                                             containerOringGrooveInnerRadius+containerOringGrooveWidth,
                                             containerOringGrooveDepth/2.0,0.0,CLHEP::twopi);

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volume
      G4VSolid* lowerContainerSolid = new G4UnionSolid(prefix+"lower_container_solid",
                                                       containerSolid1,containerSolid2,noRotation,
                                                       G4ThreeVector(0.,0.,containerHeight/2.+containerThickness/2.));//add base and walls
      G4VSolid* midContainerSolid = new G4SubtractionSolid(prefix+"mid_container_solid",
                                                           containerSolid3,containerSolid4,noRotation,
                                                           G4ThreeVector(0.,0.,0.));//remove square hole

        G4VSolid* upperContainerSolid = new G4UnionSolid(prefix+"upper_container_solid",
                                                         containerSolid5,containerSolid6,noRotation,
                                                         G4ThreeVector(0.,0.,containerSlopeHeight/2.
                                                                       +containerUpperHeight/2.));//add slope to flange
        upperContainerSolid = new G4UnionSolid(prefix+"upper_container_solid",
                                               upperContainerSolid,containerSolid7,noRotation,
                                               G4ThreeVector(0.,0.,containerUpperHeight/2.+
                                                             containerSlopeHeight+containerFlangeHeight/2.));//add container flange
        upperContainerSolid = new G4SubtractionSolid(prefix+"upper_container_solid",
                                                     upperContainerSolid,containerSolid8, noRotation,
                                                     G4ThreeVector(0.,0.,containerUpperHeight/2.+containerSlopeHeight+
                                                                   containerFlangeBaseHeight+containerNutGrooveHeight/2.));//remove groove from flange
        upperContainerSolid = new G4SubtractionSolid(prefix+"upper_container_solid",
                                                     upperContainerSolid,containerSolid9,noRotation,
                                                     G4ThreeVector(0.,0.,containerUpperHeight/2.+containerSlopeHeight+
                                                                   containerFlangeHeight-containerOringGrooveDepth/2.));

        if(screwsEnable){    // Place the screw holes by subtracting them
           G4VSolid* containerScrewHoleSolid = new G4Tubs(
                                                         prefix+"container_screw_hole_solid",0.0,
                                                         containerScrewHoleRadius,
                                                         (containerFlangeHeight-containerFlangeBaseHeight)/2.0,
                                                         0.0,CLHEP::twopi);
           G4ThreeVector screwHoleTranslation(screwDistanceFromCentre,0.,
                                             containerUpperHeight+containerSlopeHeight+
                                             containerFlangeBaseHeight+(containerNutGrooveHeight+1)/2.);

           for(int i=0; i<nScrews; i++){
             screwHoleTranslation =
               screwHoleTranslation.rotateZ(CLHEP::twopi/double(nScrews));
             upperContainerSolid = new G4SubtractionSolid(prefix+"upper_container_solid"
                                                          ,upperContainerSolid,containerScrewHoleSolid,noRotation,
                                                          screwHoleTranslation);
          }
        }

        G4VSolid* collarScrewHoleSolid = new G4Tubs(prefix+"container_collar_hole_solid",0.0,
                                                    containerScrewHoleRadius,(containerCollarHeight+1)/2.0,
                                                    0.0,CLHEP::twopi);
        //holes in the four corners of the square hole
        midContainerSolid = new G4SubtractionSolid(prefix+"mid_container_solid",
                                                   midContainerSolid,collarScrewHoleSolid,noRotation,
                                                   G4ThreeVector(containerCollarHoleWidth/2.,containerCollarHoleWidth/2.,0.));
        midContainerSolid = new G4SubtractionSolid(prefix+"mid_container_solid",
                                                   midContainerSolid,collarScrewHoleSolid,noRotation,
                                                   G4ThreeVector(-containerCollarHoleWidth/2.,containerCollarHoleWidth/2.,0.));
        midContainerSolid = new G4SubtractionSolid(prefix+"container_solid",
                                                   midContainerSolid,collarScrewHoleSolid,noRotation,
                                                   G4ThreeVector(containerCollarHoleWidth/2.,-containerCollarHoleWidth/2.,0.));
        midContainerSolid = new G4SubtractionSolid(prefix+"container_solid",
                                                   midContainerSolid,collarScrewHoleSolid,noRotation,
                                                   G4ThreeVector(-containerCollarHoleWidth/2.,-containerCollarHoleWidth/2.,0.));

        // The logical and physical volumes
        G4LogicalVolume* lowerContainerLog = new G4LogicalVolume(lowerContainerSolid,
                                      containerMaterial,prefix+"lower_container_log");
        SetColor(table,"container_colour",lowerContainerLog);

        G4ThreeVector lowerContainerPosition(samplePosition.x(),samplePosition.y(),
                                             samplePosition.z()-containerOffset-containerThickness/2.-containerHeight-
                                             containerCollarHeight+copperBoxHeight-copperBoxGap-scintThickness/2.);
        G4Transform3D lowerContainerTransform(*noRotation,lowerContainerPosition);
        // Place the container relative to the source position
        G4PVPlacementWithCheck(lowerContainerTransform,lowerContainerLog,
                               prefix+"lower_container_phys",motherLog,pMany,pCopyNo,
                               pSurfChk);

        G4LogicalVolume* midContainerLog = new G4LogicalVolume(midContainerSolid,
                                                               containerMaterial,prefix+"mid_container_log");
        SetColor(table,"container_colour",midContainerLog);

        G4ThreeVector midContainerPosition(samplePosition.x(),samplePosition.y(),
                                           lowerContainerPosition.z()+containerThickness/2.+containerHeight+containerCollarHeight/2.);
        G4Transform3D midContainerTransform(*noRotation,midContainerPosition);
        // Place the container relative to the source position
        G4PVPlacementWithCheck(midContainerTransform,midContainerLog,
                               prefix+"mid_container_phys",motherLog,pMany,pCopyNo,
                               pSurfChk);

        G4LogicalVolume* upperContainerLog = new G4LogicalVolume(upperContainerSolid,
                                                                 containerMaterial,prefix+"upper_container_log");
        SetColor(table,"container_colour",upperContainerLog);

        G4ThreeVector upperContainerPosition(samplePosition.x(),samplePosition.y(),
                                             midContainerPosition.z()+containerCollarHeight/2.+containerUpperHeight/2.);
        G4Transform3D upperContainerTransform(*noRotation,upperContainerPosition);
        // Place the container relative to the source position
        G4PVPlacementWithCheck(upperContainerTransform,upperContainerLog,
                               prefix+"upper_container_phys",motherLog,pMany,pCopyNo,
                               pSurfChk);


     // O-ring (completely fills the o-ring groove)
        G4VSolid* oringSolid = new G4Tubs(prefix+"oring_solid",
                                          containerOringGrooveInnerRadius,
                                          containerOringGrooveInnerRadius+containerOringGrooveWidth,
                                          containerOringGrooveDepth/2.0,0.0,CLHEP::twopi);
        G4LogicalVolume* oringLog = new G4LogicalVolume(oringSolid,
                                                        oringMaterial,prefix+"oring_log");

        SetColor(table,"oring_colour",oringLog);

        // Place the o-oring relative to the container
        G4ThreeVector oringPosition(samplePosition.x(),samplePosition.y(),
                                    upperContainerPosition.z()+containerUpperHeight/2.+containerSlopeHeight+
                                    containerFlangeHeight-containerOringGrooveDepth/2.);
        G4Transform3D oringTransform(*noRotation,oringPosition);
        G4PVPlacementWithCheck(oringTransform,oringLog,prefix+"oring_phys",
                                 motherLog,pMany,pCopyNo,pSurfChk);

        //Copper box
        //Make the copper box out of a series of additions and subtractions
        // Begin with all of the pieces that will make the final volume
        G4VSolid* copperSolid1 = new G4Box(prefix+"copper_solid1",copperBoxWidth/2.,
                                           copperBoxWidth/2.,copperBoxHeight/2.-copperBoxThickness/2.);//create box
        G4VSolid* copperSolid2 = new G4Box(prefix+"copper_solid2",copperBoxWidth/2.-copperBoxThickness,
                                           copperBoxWidth/2.-copperBoxThickness,copperBoxHeight/2.-copperBoxThickness);//create box void
        G4VSolid* copperSolid3 = new G4Box(prefix+"copper_solid3",copperBoxWidth/2.,
                                           copperBoxWidth/2.,copperBoxThickness/2.);//create bottom
        G4VSolid* copperSolid4 = new G4Tubs(prefix+"copper_solid4",0.,
                                            copperBoxFlangeRadius,copperBoxFlangeHeight/2.,0.,CLHEP::twopi);//create bottom flange
        G4VSolid* copperSolid5 = new G4Box(prefix+"copper_solid5",copperBoxFlangeLipWidth/2.,copperBoxFlangeLipWidth/2.,
                                           copperBoxFlangeLipHeight/2.);//create bottom flange lip
        G4VSolid* copperSolid6 = new G4Box(prefix+"copper_solid6",copperBoxFlangeLipWidth/2.-copperBoxFlangeLipThickness,
                                           copperBoxFlangeLipWidth/2.-copperBoxFlangeLipThickness,
                                           (copperBoxFlangeLipHeight+copperBoxFlangeHeight)/2.);//create void in bottom flange and lip
        G4VSolid* copperSolid7 = new G4Tubs(prefix+"copper_solid7",copperBoxMetalRadius, copperBoxFlangeRadius,
                                            copperBoxFlangeHeight/2.,0.,CLHEP::twopi);//top flange
        G4VSolid* copperSolid8 = new G4Tubs(prefix+"copper_solid8",copperBoxGlassRadius,copperBoxMetalRadius,
                                            (copperBoxGlassHeight+copperBoxFlangeHeight)/2.,0.,CLHEP::twopi);//Metal around glass plug
        G4VSolid* copperSolid9 = new G4Tubs(prefix+"copper_solid9",copperBoxOringInnerRad,copperBoxOringOuterRad,
                                            (indiumDepthBottom-indiumDepthTop)/2.,0,CLHEP::twopi);


        // Now add/subtract volumes to make the copper box, noting that the first
        // volume specified is the reference for each subsequent volume
        G4VSolid* copperSolid = new G4UnionSolid(prefix+"copper_solid",
                                                 copperSolid3,copperSolid1,noRotation,
                                                 G4ThreeVector(0.,0.,(copperBoxHeight-copperBoxThickness)/2.));//add base to walls
        copperSolid = new G4SubtractionSolid(prefix+"copper_solid",copperSolid,copperSolid2,noRotation,
                                             G4ThreeVector(0.,0.,(copperBoxHeight-copperBoxThickness)/2.));//take void away from box
        copperSolid = new G4UnionSolid(prefix+"copper_solid",
                                       copperSolid,copperSolid4,noRotation,
                                       G4ThreeVector(0.,0.,copperBoxHeight+copperBoxFlangeHeight/2.));//add bottom flange

        copperSolid = new G4UnionSolid(prefix+"copper_solid",
                                       copperSolid,copperSolid5,noRotation,G4ThreeVector(0.,0.,
                                        copperBoxHeight-copperBoxFlangeLipHeight/2.));//add lower lip
        copperSolid = new G4SubtractionSolid(prefix+"copper_solid",copperSolid,copperSolid6, noRotation,
                                             G4ThreeVector(0,0,(copperBoxHeight-(copperBoxFlangeLipHeight+copperBoxFlangeHeight)/2.)));// take void away from lip and flange
        copperSolid = new G4UnionSolid(prefix+"copper_solid",copperSolid,copperSolid7, noRotation,
                                       G4ThreeVector(0.,0.,copperBoxHeight+copperBoxFlangeHeight+copperBoxFlangeHeight/2.));//add top flange
        copperSolid = new G4UnionSolid(prefix+"copper_solid",copperSolid,copperSolid8, noRotation,
                                       G4ThreeVector(0.,0.,copperBoxHeight+copperBoxFlangeHeight+
                                                     copperBoxFlangeHeight/2.+copperBoxGlassHeight/2.));//add metal around glass
        copperSolid = new G4SubtractionSolid(prefix+"copper_solid",copperSolid,copperSolid9,noRotation,
                                             G4ThreeVector(0.,0.,copperBoxHeight+copperBoxFlangeHeight
                                                           -indiumDepthBottom+(indiumDepthBottom-indiumDepthTop)/2.));//remove indium flange gap

        G4LogicalVolume* copperLog = new G4LogicalVolume(copperSolid,copperMaterial,
                                                         prefix+"copper_log");
        SetColor(table,"copper_colour",copperLog);

        // Position the copperbox relative to the container position
        G4ThreeVector copperPosition(samplePosition.x(),samplePosition.y(),
                                     lowerContainerPosition.z()+containerThickness/2.+containerHeight+containerCollarHeight
                                     -copperBoxHeight+copperBoxGap);
        G4Transform3D copperTransform(*noRotation,copperPosition);

        G4PVPlacementWithCheck(copperTransform,copperLog,prefix+"copper_phys",
                               motherLog,pMany,pCopyNo,pSurfChk);

        //glass plug
        G4VSolid* glassSolid1 = new G4Tubs(prefix+"glass_solid1",0.,
                                           copperBoxGlassRadius,(copperBoxGlassHeight+copperBoxFlangeHeight)/2.,
                                           0., CLHEP::twopi);//create glass plug
        //place plug
        G4LogicalVolume* glassLog = new G4LogicalVolume(glassSolid1,glassMaterial,
                                                        prefix+"glass_log");
        SetColor(table,"glass_colour",glassLog);

        // Position the glass relative to the container position
        G4ThreeVector glassPosition(samplePosition.x(),samplePosition.y(),
                                    copperPosition.z()+copperBoxHeight+copperBoxFlangeHeight
                                    +(copperBoxFlangeHeight+copperBoxGlassHeight)/2.);
        G4Transform3D glassTransform(*noRotation,glassPosition);
        G4PVPlacementWithCheck(glassTransform,glassLog,prefix+"glass_phys",
                               motherLog,pMany,pCopyNo,pSurfChk);

        // Indium O-ring (completely fills the copper box o-ring groove)
        G4VSolid* copperOringSolid = new G4Tubs(prefix+"copper_oring_solid",
                                                copperBoxOringInnerRad,copperBoxOringOuterRad,
                                                (indiumDepthBottom-indiumDepthTop)/2.,0.0,CLHEP::twopi);
        G4LogicalVolume* copperOringLog = new G4LogicalVolume(copperOringSolid,
                                                              indiumMaterial,prefix+"copper_oring_log");
        SetColor(table,"indium_colour",copperOringLog);

        // Place the Indium o-oring relative to the container
        G4ThreeVector copperOringPosition(samplePosition.x(),samplePosition.y(),
                                          copperPosition.z()+copperBoxHeight+copperBoxFlangeHeight-
                                          indiumDepthBottom+(indiumDepthBottom-indiumDepthTop)/2.);
        G4Transform3D copperOringTransform(*noRotation,copperOringPosition);
        G4PVPlacementWithCheck(copperOringTransform,copperOringLog,prefix+"copper_oring_phys",
                               motherLog,pMany,pCopyNo,pSurfChk);

        // Stem
        // Make the stem out of a series of additions and subtractions, since
        // it is a cylinder with varying inner/outer radii
        // Begin with all of the pieces that will make the final volume
        G4VSolid* stemSolid1 = new G4Tubs(prefix+"stem_solid1",boreRadius,
                                          stemFlangeRadius,stemFlangeThickness/2.0,0.0,CLHEP::twopi);
        G4VSolid* stemSolid2 = new G4Tubs(prefix+"stem_solid2",boreRadius,
                                          stemFlangeEndRadius,stemFlangeEndLength/2.0,0.0,CLHEP::twopi);
        G4VSolid* stemSolid3 = new G4Cons(prefix+"stem_solid3",boreRadius,
                                          stemFlangeEndRadius,boreRadius,stemConnectorEndRadius,
                                          stemAngledLength/2.0,0.0,CLHEP::twopi);//angled part of stem
        G4VSolid* stemSolid4 = new G4Tubs(prefix+"stem_solid4",boreRadius,
                                          stemConnectorEndRadius,stemConnectorEndLength/2.0,
                                          0.0,CLHEP::twopi);
        G4VSolid* stemSolid5 = new G4Tubs(prefix+"stem_solid5",0.0,
                                          connectorRadius,connectorThickness/2.0,0.0,CLHEP::twopi);

        // Now add/subtract volumes to make the stem, noting that the first
        // volume specified is the reference for each subsequent volume
        G4VSolid* stemSolid = new G4UnionSolid(prefix+"stem_solid",stemSolid1,
                                               stemSolid2,noRotation,
                                               G4ThreeVector(0.,0.,(stemFlangeThickness+stemFlangeEndLength)/2.0));
        stemSolid = new G4UnionSolid(prefix+"stem_solid",stemSolid,stemSolid3,
                             noRotation,G4ThreeVector(0.,0.,stemFlangeEndLength+
                             (stemFlangeThickness+stemAngledLength)/2.0));
        stemSolid = new G4UnionSolid(prefix+"stem_solid",stemSolid,stemSolid4,
                             noRotation,G4ThreeVector(0.,0.,stemFlangeEndLength+
                             stemAngledLength+stemFlangeThickness/2.0+
                             stemConnectorEndLength/2.0));
        stemSolid = new G4UnionSolid(prefix+"stem_solid",stemSolid,stemSolid5,
                             noRotation,G4ThreeVector(0.,0.,stemLength-
                             (stemFlangeThickness+connectorThickness)/2.0));
        // Now place the screw holes by subtracting them. There are two holes,
        // one for the screw head and one for the body.
        if(screwsEnable){
            G4VSolid* stemScrewHoleSolid = new G4Tubs(
                                                      prefix+"stem_screw_hole_solid",0.0,stemScrewHoleRadius,
                                                      (stemFlangeThickness+1)/2.0,0.0,CLHEP::twopi);

            G4ThreeVector screwHoleTranslation(screwDistanceFromCentre,0.,
                                               0);
            for(int i=0; i<nScrews; i++){
                screwHoleTranslation =
                screwHoleTranslation.rotateZ(CLHEP::twopi/double(nScrews));
                stemSolid = new G4SubtractionSolid(prefix+"stem_solid",
                                                   stemSolid,stemScrewHoleSolid,noRotation,
                                                   screwHoleTranslation);
            }
        }

        G4LogicalVolume* stemLog = new G4LogicalVolume(stemSolid,stemMaterial,
                                                       prefix+"stem_log");
        SetColor(table,"stem_colour",stemLog);

        // Position the stem relative to the container position
        G4ThreeVector stemPosition(samplePosition.x(),samplePosition.y(),
                                   upperContainerPosition.z()
                                   +containerUpperHeight/2.+containerFlangeHeight+containerSlopeHeight+stemFlangeThickness/2.);
        G4Transform3D stemTransform(*noRotation,stemPosition);
        G4PVPlacementWithCheck(stemTransform,stemLog,prefix+"stem_phys",
                                   motherLog,pMany,pCopyNo,pSurfChk);

      //Fill space in stem with air
      G4VSolid* airStemSolid1 = new G4Tubs(prefix+"air_stem_solid",0.0,
                                        boreRadius,stemFlangeThickness/2.0,0.0,CLHEP::twopi);
      G4VSolid* airStemSolid2 = new G4Tubs(prefix+"air_stem_solid2",0.0,
                                        boreRadius,stemFlangeEndLength/2.0,0.0,CLHEP::twopi);
      G4VSolid* airStemSolid3 = new G4Tubs(prefix+"air_stem_solid3",0.0,
                                        boreRadius,stemAngledLength/2.0,0.0,CLHEP::twopi);
      G4VSolid* airStemSolid4 = new G4Tubs(prefix+"air_stem_solid4",0.0,
                                        boreRadius,stemConnectorEndLength/2.0,
                                        0.0,CLHEP::twopi);

      // Now add/subtract volumes to make the stem, noting that the first
      // volume specified is the reference for each subsequent volume
      G4VSolid* airStemSolid = new G4UnionSolid(prefix+"air_stem_solid",airStemSolid1,
                                             airStemSolid2,noRotation,
                                             G4ThreeVector(0.,0.,(stemFlangeThickness+stemFlangeEndLength)/2.0));
      airStemSolid = new G4UnionSolid(prefix+"air_stem_solid",airStemSolid,airStemSolid3,
                                   noRotation,G4ThreeVector(0.,0.,stemFlangeEndLength+
                                                            (stemFlangeThickness+stemAngledLength)/2.0));
      airStemSolid = new G4UnionSolid(prefix+"air_stem_solid",airStemSolid,airStemSolid4,
                                   noRotation,G4ThreeVector(0.,0.,stemFlangeEndLength+
                                                            stemAngledLength+stemFlangeThickness/2.0+
                                                            stemConnectorEndLength/2.0));

      G4LogicalVolume* airStemLog = new G4LogicalVolume(airStemSolid,airMaterial,
                                                     prefix+"air_stem_log");
      SetColor(table,"air_colour",airStemLog);

      // Position the air in the stem
      G4Transform3D airStemTransform(*noRotation,stemPosition);
      G4PVPlacementWithCheck(stemTransform,airStemLog,prefix+"air_stem_phys",
                             motherLog,pMany,pCopyNo,pSurfChk);

      //fill the lower container with air
      G4VSolid* airContainerSolid1 = new G4Tubs(prefix+"air_container_solid1",
                                                0.0,containerRadius-containerThickness,
                                                containerHeight/2.0,0.0,CLHEP::twopi);
      G4VSolid* airContainerSolid2 = new G4Box(prefix+"air_container_solid2",copperBoxWidth/2.,
                                               copperBoxWidth/2.,copperBoxHeight/2.);//create copper box hole
      //remove the copper box from the air volume
      G4VSolid* airContainerSolid = new G4SubtractionSolid(prefix+"air_container_solid",
                                                           airContainerSolid1,airContainerSolid2,noRotation,
                                                           G4ThreeVector(0.,0.,containerHeight/2-airCopperRemoval));
      // The logical and physical volumes
      G4LogicalVolume* airContainerLog = new G4LogicalVolume(airContainerSolid,
                                                             airMaterial,prefix+"air_container_log");
      SetColor(table,"air_colour",airContainerLog);

      G4ThreeVector airContainerPosition(samplePosition.x(),samplePosition.y(),
                                         samplePosition.z()-containerOffset+containerHeight/2.-scintThickness/2.-containerThickness/2.);
      G4Transform3D airContainerTransform(*noRotation,airContainerPosition);
      // Place the container relative to the source position
      G4PVPlacementWithCheck(airContainerTransform,airContainerLog,
                             prefix+"air_container_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);


     // Screws and nuts (if enabled)
        if(screwsEnable){
            // The screws
            G4VSolid* screwSolid = new G4Tubs(prefix+"screw_solid",0.0,
                                            screwRadius,screwLength/2.0,
                                            0.0,CLHEP::twopi);
            G4VSolid* screwHeadSolid = new G4Tubs(prefix+"screw_head_solid",0.0,
                                            screwHeadRadius,screwHeadLength/2.0,
                                            0.0,CLHEP::twopi);
            screwSolid = new G4UnionSolid(prefix+"screw_solid",screwSolid,
                                            screwHeadSolid,noRotation,
                                            G4ThreeVector(0.,0.,
                                           (screwLength-screwHeadLength)/2.0));
            G4LogicalVolume* screwLog = new G4LogicalVolume(screwSolid,
                                            screwMaterial,prefix+"screw_log");
            SetColor(table,"screw_colour",screwLog);

            // Place the screws relative to the stem position
            G4ThreeVector screwPosition(samplePosition.x()+
                                 screwDistanceFromCentre,samplePosition.y(),
                                 stemPosition.z()+
                                (stemFlangeThickness-screwLength)/2.+
                                 screwHeadLength);
            // The nuts
            G4VSolid* nutSolid = new G4Tubs(prefix+"nut_solid",
                                        screwRadius+nutInsertThickness,
                                        nutRadius,nutThickness/2.0,0.0,CLHEP::twopi);
            G4LogicalVolume* nutLog = new G4LogicalVolume(nutSolid,nutMaterial,
                                                      prefix+"nut_log");
            SetColor(table,"nut_colour",nutLog);
            G4VSolid* nutInsertSolid = new G4Tubs(prefix+"nut_insert_solid",
                             screwRadius,screwRadius+nutInsertThickness,
                             nutThickness/2.0,0.0,CLHEP::twopi);
            G4LogicalVolume* nutInsertLog = new G4LogicalVolume(nutInsertSolid,
                                     nutInsertMaterial,prefix+"nut_insert_log");
            SetColor(table,"nut_insert_colour",nutInsertLog);
            // Place the nuts relative to the stem position
            G4ThreeVector nutPosition(screwPosition.x(),screwPosition.y(),
                                      upperContainerPosition.z()+containerUpperHeight/2.+containerSlopeHeight+containerFlangeBaseHeight+containerNutGrooveHeight/2.);

            // Physically place the nuts and screws
            for(int i=0; i<nScrews; i++){
                screwPosition = screwPosition.rotateZ(CLHEP::twopi/double(nScrews));
                nutPosition = nutPosition.rotateZ(CLHEP::twopi/double(nScrews));
                G4Transform3D screwTransform(*noRotation,screwPosition);
                G4Transform3D nutTransform(*noRotation,nutPosition);
                G4Transform3D nutInsertTransform(*noRotation,nutPosition);
                G4PVPlacementWithCheck(screwTransform,screwLog,
                               prefix+"screw_phys_"+to_string(i),motherLog,
                               pMany,pCopyNo,pSurfChk);
                G4PVPlacementWithCheck(nutTransform,nutLog,
                               prefix+"nut_phys_"+to_string(i),motherLog,
                               pMany,pCopyNo,pSurfChk);
                G4PVPlacementWithCheck(nutInsertTransform,nutInsertLog,
                               prefix+"nut_insert_phys_"+to_string(i),
                               motherLog,pMany,pCopyNo,pSurfChk);
            }
        }

     // PMT
        // The PMT body (metal enclosure)
        G4VSolid* pmtSolid = new G4Box(prefix+"pmt_solid",pmtFaceLength/2.0,
                                        pmtFaceLength/2.0,pmtLength/2.0);
        G4VSolid* pmtInsetSolid = new G4Tubs(prefix+"pmt_inset_solid",0.0,
                                        pmtWindowRadius,
                                        (pmtWindowInset+pmtFaceThickness)/2.0,
                                        0.0,CLHEP::twopi);
        pmtSolid = new G4SubtractionSolid(prefix+"pmt_solid",pmtSolid,
                             pmtInsetSolid,noRotation,G4ThreeVector(0.,0.,
                             -(pmtLength-pmtWindowInset-pmtFaceThickness)/2.0));

        G4LogicalVolume* pmtLog = new G4LogicalVolume(pmtSolid,pmtMaterial,
                                                      prefix+"pmt_log");
        SetColor(table,"pmt_colour",pmtLog);

        // Position the PMT relative to the scintillator
        G4ThreeVector pmtPosition(samplePosition.x(),samplePosition.y(),
                                  copperPosition.z()+pmtLength/2.+(copperBoxThickness+scintThickness+pmtWindowInset)/2.);
        G4Transform3D pmtTransform(*noRotation,pmtPosition);
        G4PVPlacementWithCheck(pmtTransform,pmtLog,prefix+"pmt_phys",
                               motherLog,pMany,pCopyNo,pSurfChk);

        // The non-active part of the PMT face
        G4VSolid* pmtFaceSolid = new G4Tubs(prefix+"pmt_face_solid",
                                            pmtActiveRadius,pmtWindowRadius,
                                            pmtFaceThickness/2.0,0.0,CLHEP::twopi);
        G4LogicalVolume* pmtFaceLog = new G4LogicalVolume(pmtFaceSolid,
                                     pmtActiveMaterial,prefix+"pmt_face_log");
        SetColor(table,"pmt_colour",pmtFaceLog);

        G4ThreeVector pmtFacePosition(samplePosition.x(),samplePosition.y(),
                                      copperPosition.z()+(scintThickness+pmtFaceThickness));
        G4Transform3D pmtFaceTransform(*noRotation,pmtFacePosition);
        G4PVPlacementWithCheck(pmtFaceTransform,pmtFaceLog,
                   prefix+"pmt_face_phys",motherLog,pMany,pCopyNo,pSurfChk);

        // The active part of the PMT face
        G4VSolid* pmtActiveSolid = new G4Tubs(prefix+"pmt_active_solid",
                                             0.0,pmtActiveRadius,
                                             pmtFaceThickness/2.0,0.0,CLHEP::twopi);
        G4LogicalVolume* pmtActiveLog = new G4LogicalVolume(pmtActiveSolid,
                                   pmtActiveMaterial,prefix+"pmt_active_log");
        SetColor(table,"pmt_colour",pmtActiveLog);

        G4ThreeVector pmtActivePosition(samplePosition.x(),samplePosition.y(),
                                        copperPosition.z()+copperBoxThickness+(scintThickness+pmtFaceThickness));
        G4Transform3D pmtActiveTransform(*noRotation,pmtActivePosition);
        G4PVPlacementWithCheck(pmtActiveTransform,pmtActiveLog,
                 prefix+"pmt_active_phys",motherLog,pMany,pCopyNo,pSurfChk);

        // Scintillator button
        G4VSolid* scintSolid = new G4Tubs(prefix+"scintillator_solid",0.0,
                                          scintRadius,scintThickness/2.0,0.0,CLHEP::twopi);
        G4LogicalVolume* scintLog = new G4LogicalVolume(scintSolid,
                                      scintMaterial,prefix+"scintillator_log");
        SetColor(table,"scintillator_colour",scintLog);

        // Make the scintillator sensitive (the PMT will record a photoelectron
        // based on a non-zero energy deposition in the scintillator)
        G4SDManager* sDManager = G4SDManager::GetSDMpointer();
        CalibPMTSD* pmtSD = new CalibPMTSD(detectorName,lcn,pmtEnergyThreshold,
                                           pmtEfficiency);
        sDManager->AddNewDetector(pmtSD);
        scintLog->SetSensitiveDetector(pmtSD);

        // Place the scintillator (its centre is where the source is, by
        // assumption)
        G4ThreeVector scintPosition(samplePosition.x(),samplePosition.y(),
                                    copperPosition.z()+scintThickness/2.+copperBoxThickness);
        G4Transform3D scintTransform(*noRotation,scintPosition);
        G4PVPlacementWithCheck(scintTransform,scintLog,
                                    prefix+"scintillator_phys",motherLog,
                                    pMany,pCopyNo,pSurfChk);

    }
    catch(DBNotFoundError &e) {
        Log::Die("GeoTaggedSourceFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field + ".");
    };
  }
} // namespace RAT
