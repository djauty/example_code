////////////////////////////////////////////////////////////////////////
// Last svn revision: $Id$
////////////////////////////////////////////////////////////////////////

#include <RAT/GeoUFOFactory.hh>

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
#include "G4UnitsTable.hh"

#include <vector>
#include <string>
#include <algorithm>

namespace RAT
{
  void GeoUFOFactory::SetColor(DBLinkPtr table, G4String colourName, G4LogicalVolume *logicalVolume)
  {
    // Set the color of a logical volume

    G4VisAttributes *vis = new G4VisAttributes();
    try {
      const std::vector<double> &color = table->GetDArray(colourName);
      Log::Assert(color.size() == 3 || color.size() == 4, "GeoUFOFactory: Colour entry " + colourName + " does not have 3 (RGB) or 4 (RGBA) components");
      if(color.size() == 3) // RGB
        vis->SetColour(G4Colour(color[0], color[1], color[2]));
      else if(color.size() == 4) // RGBA
        vis->SetColour(G4Colour(color[0], color[1], color[2], color[3]));
    }
    catch(DBNotFoundError &e){
      Log::Die("GeoUFOFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field +".");
    };

    logicalVolume->SetVisAttributes(vis);
  } // SetColour

  std::vector<double> GeoUFOFactory::MultiplyVectorByUnit(std::vector<double> v, const double unit) {
    transform(v.begin(),v.end(),v.begin(),bind2nd(std::multiplies<double>(),unit));
    return v;
  } // MultiplyVectorByUnit

  G4PVPlacement* GeoUFOFactory::G4PVPlacementWithCheck(
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
                "GeoUFOFactory: Overlap detected when placing volume " +
                pName + ". See log for details.");
    return placement;
  } // G4PVPlacementWithCheck


  void GeoUFOFactory::Construct(DBLinkPtr table,
                                const bool checkOverlaps)
  {
    // Define a default rotation matrix that does not rotate, and other constants
    G4RotationMatrix *noRotation = new G4RotationMatrix();
    const bool pMany = false;
    const int pCopyNo = 0;

    // Get a link to the database storing the source PMT geometry information
    DBLinkPtr ufoTable = DB::Get()->GetLink("UFO","ufo");

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
                  "GeoUFOFactory: Unable to find mother volume '" +
                  motherName + "' for '" + index + "'.");

      // Get the position of the electronics and build the volume relative to it
      std::vector<double> const &pos =
        MultiplyVectorByUnit(table->GetDArray("sample_position"),CLHEP::mm);
      Log::Assert(pos.size() == 3,"GeoUFOFactory: sample_position does not have three components.");
      G4ThreeVector samplePosition(pos[0], pos[1], pos[2]);

      // =====================================
      // Read all parameters from the database
      // =====================================

      // Container parameters
      const double acrylicRadius = table->GetD("acrylic_radius") * CLHEP::mm;
      const double acrylicHeight = table->GetD("acrylic_height") * CLHEP::mm;
      const double acrylicInnerRad = table->GetD("acrylic_inner_rad") * CLHEP::mm;
      const double acrylicCollarHeight = table->GetD("acrylic_collar_height") * CLHEP::mm;
      const double acrylicCollarRad = table->GetD("acrylic_collar_rad")*CLHEP::mm;
      const double acrylicOringGrooveThickness = table->GetD("acrylic_oring_groove_thickness") * CLHEP::mm;
      const double acrylicOringGrooveRad = table->GetD("acrylic_oring_groove_rad") * CLHEP::mm;
      const double acrylicOringGrooveHeight = table->GetD("acrylic_oring_groove_height") * CLHEP::mm;
      const double acrylicLEDHeight = table->GetD("acrylic_LED_height") * CLHEP::mm;

      // UFO cap
      const double inch = 25.4; //in the blue print measurements are in inches keep so can see converstion
      const double capInnerRadius = table->GetD("cap_inner_rad") * inch * CLHEP::mm;
      const double capRadius = table->GetD("cap_radius") * inch * CLHEP::mm;
      const double capThickness = table->GetD("cap_thickness") * inch * CLHEP::mm;
      const double capSpaceRadius = table->GetD("cap_space_rad") * inch * CLHEP::mm;
      const double capSpaceThickness = table->GetD("cap_space_thk") * inch * CLHEP::mm;

      //Bottom connection to UFO
      const double bottomCupRadius = table->GetD("bottom_cup_radius") * inch * CLHEP::mm;
      const double bottomCupHeight = table->GetD("bottom_cup_height") * inch * CLHEP::mm;
      const double bottomCupTopInnerRadius = table->GetD("bottom_cup_top_inner_rad") * inch * CLHEP::mm;
      const double bottomCupTopHeight= table->GetD("bottom_cup_top_height") * inch * CLHEP::mm;
      const double bottomCupMidInnerRadius= table->GetD("bottom_cup_mid_inner_rad") * inch * CLHEP::mm;
      const double bottomCupMidHeight= table->GetD("bottom_cup_mid_height") * inch * CLHEP::mm;
      const double bottomCupBotInnerRadius= table->GetD("bottom_cup_bot_inner_rad") * inch * CLHEP::mm;
      const double bottomCupBotOuterRadius= table->GetD("bottom_cup_bot_outer_rad") * inch * CLHEP::mm;
      const double bottomCupBotOuterHeight= table->GetD("bottom_cup_bot_outer_height") * inch * CLHEP::mm;
      const double bottomCupMidTopHeight= table->GetD("bottom_cup_mid_top_height") * inch * CLHEP::mm;
      const double bottomCupGap= table->GetD("bottom_cup_gap") * inch * CLHEP::mm;

      //Bottom disc
      const double bottomDiscRadius = table->GetD("bottom_disc_radius") * CLHEP::mm;
      const double bottomDiscInnerRadius = table->GetD("bottom_disc_inner_radius") * CLHEP::mm;
      const double bottomDiscThickness= table->GetD("bottom_disc_thickness") * CLHEP::mm;
      const double bottomDiscHoleRadius= table->GetD("bottom_disc_hole_radius") * CLHEP::mm;
      const double bottomDiscDistanceRad= table->GetD("bottom_disc_distance_rad") * CLHEP::mm;

      //electronics
      const double electronicsRadius = table->GetD("electronics_rad") * CLHEP::mm;
      const double electronicsThickness = table->GetD("electronics_thk") * CLHEP::mm;


      G4Material* const acrylicMaterial =
        G4Material::GetMaterial(table->GetS("acrylic_material"));
      G4Material* const oringMaterial =
        G4Material::GetMaterial(table->GetS("oring_material"));
      G4Material* const electronicsMaterial =
        G4Material::GetMaterial(table->GetS("electronics_material"));
      G4Material* const capMaterial =
        G4Material::GetMaterial(table->GetS("cap_material"));
      G4Material* const bottomCupMaterial =
        G4Material::GetMaterial(table->GetS("bottom_cup_material"));
      G4Material* const bottomDiscMaterial =
        G4Material::GetMaterial(table->GetS("bottom_disc_material"));
      G4Material* const airMaterial =
        G4Material::GetMaterial(table->GetS("air_material"));



      // ===================================
      // Build the solid and logical volumes
      // and place them physically
      // ===================================



      // Container
      // Make the container out of a series of additions and subtractions,
      // since it is a cylinder with varying inner/outer radii
      // Begin with all of the pieces that will make the final volume

      G4VSolid* acrylicSolid1 = new G4Tubs(prefix+"acrylic_solid1",acrylicInnerRad,
                                             acrylicRadius,acrylicHeight/2.0,0.0,CLHEP::twopi);//acrylic walls
      G4VSolid* acrylicSolid2 = new G4Tubs(prefix+"acrylic_solid2",
                                             acrylicCollarRad,
                                             acrylicRadius+.1,acrylicCollarHeight/2.0,0.0,CLHEP::twopi);//acrylic collar
      G4VSolid* acrylicSolid3 = new G4Tubs(prefix+"acrylic_solid3",
                                             acrylicOringGrooveRad,acrylicCollarRad+.1,
                                             acrylicOringGrooveThickness/2.0,0.0,CLHEP::twopi);//oring groove

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volume

      G4VSolid* acrylicSolid= new G4SubtractionSolid(prefix+"acrylic_solid",
                                                     acrylicSolid1,acrylicSolid2,noRotation,
                                                     G4ThreeVector(0.,0.,-acrylicHeight/2.+acrylicCollarHeight/2.-.01));//remove collar from bottom
      acrylicSolid = new G4SubtractionSolid(prefix+"acrylic_solid",
                                            acrylicSolid,acrylicSolid2,noRotation,
                                            G4ThreeVector(0.,0.,acrylicHeight/2.-acrylicCollarHeight/2.+.01));//remove collar from top
      acrylicSolid = new G4SubtractionSolid(prefix+"acrylic_solid",
                                            acrylicSolid,acrylicSolid3,noRotation,
                                            G4ThreeVector(0.,0.,-acrylicHeight/2.+acrylicCollarHeight/2.-acrylicOringGrooveHeight));//remove o-ring from bottom
      acrylicSolid = new G4SubtractionSolid(prefix+"acrylic_solid",
                                            acrylicSolid,acrylicSolid3,noRotation,
                                            G4ThreeVector(0.,0.,acrylicHeight/2-acrylicCollarHeight/2+acrylicOringGrooveHeight));//remove o-ring from top


      // The logical and physical volumes
      G4LogicalVolume* acrylicLog = new G4LogicalVolume(acrylicSolid,
                                                        acrylicMaterial,prefix+"acrylic_log");
      SetColor(table,"acrylic_colour",acrylicLog);
      G4ThreeVector acrylicPosition(samplePosition.x(),samplePosition.y(),samplePosition.z());
      G4Transform3D acrylicTransform(*noRotation,acrylicPosition);
      G4PVPlacementWithCheck(acrylicTransform,acrylicLog,
                             prefix+"acrylic_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //oring
      G4VSolid* oringSolid = new G4Tubs(prefix+"oring_solid",acrylicOringGrooveRad,
                                        acrylicCollarRad,acrylicOringGrooveThickness/2.0,0.0,CLHEP::twopi);//oring

      // The logical and physical volumes
      G4LogicalVolume* oringLog1 = new G4LogicalVolume(oringSolid,
                                                       oringMaterial,prefix+"oring_log1");
      SetColor(table,"oring_colour",oringLog1);
      G4ThreeVector oringPosition1(acrylicPosition.x(),acrylicPosition.y(),
                                   acrylicPosition.z()-acrylicHeight/2.+acrylicCollarHeight/2.-acrylicOringGrooveHeight);
      G4Transform3D oringTransform1(*noRotation,oringPosition1);
      G4PVPlacementWithCheck(oringTransform1,oringLog1,
                             prefix+"oring_phys1",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //now the bottom oring
      G4LogicalVolume* oringLog2 = new G4LogicalVolume(oringSolid,
                                                       oringMaterial,prefix+"oring_log1");
      SetColor(table,"oring_colour",oringLog2);
      G4ThreeVector oringPosition2(acrylicPosition.x(),acrylicPosition.y(),
                                   acrylicPosition.z()+acrylicHeight/2.-acrylicCollarHeight/2+acrylicOringGrooveHeight);
      G4Transform3D oringTransform2(*noRotation,oringPosition2);
      G4PVPlacementWithCheck(oringTransform2,oringLog2,
                             prefix+"oring_phys2",motherLog,pMany,pCopyNo,
                             pSurfChk);


      // Cap
      // Make the cap out of a series of additions and subtractions,
      // since it is a cylinder with varying inner/outer radii
      // Begin with all of the pieces that will make the final volume
      G4VSolid* capSolid1 = new G4Tubs(prefix+"cap_solid1",capInnerRadius,
                                       capRadius,capThickness/2.,0.0,CLHEP::twopi);//cap metal
      G4VSolid* capSolid2 = new G4Tubs(prefix+"cap_solid2",0,
                                       capSpaceRadius,capSpaceThickness,0.0,CLHEP::twopi);//remove conector with acrylic

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volume
      G4VSolid* capSolid= new G4SubtractionSolid(prefix+"cap_solid",
                                                 capSolid1,capSolid2,noRotation,
                                                 G4ThreeVector(0.,0.,-capThickness/2.+capSpaceThickness/2.));//remove the space for the acrylic

      // The logical and physical volumes
      G4LogicalVolume* capLog = new G4LogicalVolume(capSolid,
                                                    capMaterial,prefix+"cap_log");
      SetColor(table,"cap_colour",capLog);
      G4ThreeVector capPosition(acrylicPosition.x(),acrylicPosition.y(),
                                acrylicPosition.z()+acrylicHeight/2.+capSpaceThickness/2.);
      G4Transform3D capTransform(*noRotation,capPosition);
      G4PVPlacementWithCheck(capTransform,capLog,
                             prefix+"cap_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);


      // Cap stopper this is where the pully joins just make a solid pice of metal
      // Make the cap stopper out of a series of additions and subtractions,
      // since it is a cylinder with varying inner/outer radii
      // Begin with all of the pieces that will make the final volume
      G4VSolid* capSolid3 = new G4Tubs(prefix+"cap_solid3",0.0,
                                       capInnerRadius,capThickness/2.-capSpaceThickness*3./4.,0.0,CLHEP::twopi);//cap metal


      // The logical and physical volumes
      G4LogicalVolume* capStopLog = new G4LogicalVolume(capSolid3,
                                                        capMaterial,prefix+"cap_log");
      SetColor(table,"bottom_cup_colour",capLog);
      G4ThreeVector capStopPosition(acrylicPosition.x(),acrylicPosition.y(),
                                    acrylicPosition.z()+acrylicHeight/2.+capSpaceThickness*5./4.);

      G4Transform3D capStopTransform(*noRotation,capStopPosition);
      G4PVPlacementWithCheck(capStopTransform,capStopLog,
                             prefix+"cap_stop_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      // Bottom cup
      // Make the bottom cup out of a series of additions and subtractions,
      // since it is a cylinder with varying inner/outer radii
      // Begin with all of the pieces that will make the final volume
      G4VSolid* bottomCupSolid1 = new G4Tubs(prefix+"bottom_cup_solid1",bottomCupBotInnerRadius,
                                             bottomCupRadius,bottomCupHeight/2.0,0.0,CLHEP::twopi);//bottom cup metal
      G4VSolid* bottomCupSolid2 = new G4Tubs(prefix+"bottom_cup_solid2",0.,
                                             bottomCupTopInnerRadius,bottomCupTopHeight/2.0,0.0,CLHEP::twopi);//the top space
      G4VSolid* bottomCupSolid3 = new G4Tubs(prefix+"bottom_cup_solid3",0,
                                             bottomCupMidInnerRadius,bottomCupMidHeight/2.0,0.0,CLHEP::twopi);//the mid space
      G4VSolid* bottomCupSolid4 = new G4Tubs(prefix+"bottom_cup_solid4",bottomCupBotOuterRadius,
                                             bottomCupRadius+.1,bottomCupBotOuterHeight/2.0,0.0,CLHEP::twopi);//the bot outer space

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volume
      G4VSolid* bottomCupSolid= new G4SubtractionSolid(prefix+"bottom_cup_solid",
                                                       bottomCupSolid1,bottomCupSolid2,noRotation,
                                                       G4ThreeVector(0.,0.,bottomCupHeight/2-bottomCupTopHeight/2.));
      bottomCupSolid= new G4SubtractionSolid(prefix+"bottom_cup_solid",
                                             bottomCupSolid,bottomCupSolid3,noRotation,
                                             G4ThreeVector(0.,0.,bottomCupHeight/2.-bottomCupTopHeight-bottomCupMidHeight/2.));
      bottomCupSolid= new G4SubtractionSolid(prefix+"bottom_cup_solid",
                                             bottomCupSolid,bottomCupSolid4,noRotation,
                                             G4ThreeVector(0.,0.,bottomCupHeight/2-bottomCupMidTopHeight-bottomCupBotOuterHeight/2.));

      // The logical and physical volumes
      G4LogicalVolume* bottomCupLog = new G4LogicalVolume(bottomCupSolid,
                                                          bottomCupMaterial,prefix+"bottom_cup_log");
      SetColor(table,"bottom_cup_colour",bottomCupLog);
      G4ThreeVector bottomCupPosition(acrylicPosition.x(),acrylicPosition.y(),
                                      acrylicPosition.z()-acrylicHeight/2.-bottomCupHeight/2.+acrylicCollarHeight-bottomCupGap);
      G4Transform3D bottomCupTransform(*noRotation,bottomCupPosition);
      G4PVPlacementWithCheck(bottomCupTransform,bottomCupLog,
                             prefix+"bottom_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //Bottom disc
      //Disc with holes in it
      G4VSolid* bottomDiscSolid = new G4Tubs(prefix+"bottom_disc_solid",bottomDiscInnerRadius,
                                             bottomDiscRadius,bottomDiscThickness/2.0,0.0,CLHEP::twopi);//bottom disc metal
      G4VSolid* bottomDiscHoleSolid = new G4Tubs(prefix+"bottom_disc_hole_solid",0.0,
                                                 bottomDiscHoleRadius,bottomDiscThickness/2.0,0.0,CLHEP::twopi);//bottom disc holes

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volumes
      bottomDiscSolid = new G4SubtractionSolid(prefix+"bottom_disc_solid",
                                               bottomDiscSolid,bottomDiscHoleSolid,noRotation,
                                               G4ThreeVector(bottomDiscDistanceRad,0.,0.));
      bottomDiscSolid = new G4SubtractionSolid(prefix+"bottom_disc_solid",
                                               bottomDiscSolid,bottomDiscHoleSolid,noRotation,
                                               G4ThreeVector(-bottomDiscDistanceRad,0.,0.));
      bottomDiscSolid = new G4SubtractionSolid(prefix+"bottom_disc_solid",
                                               bottomDiscSolid,bottomDiscHoleSolid,noRotation,
                                               G4ThreeVector(0.,bottomDiscDistanceRad,0.));
      bottomDiscSolid = new G4SubtractionSolid(prefix+"bottom_disc_solid",
                                               bottomDiscSolid,bottomDiscHoleSolid,noRotation,
                                               G4ThreeVector(0.,bottomDiscDistanceRad,0.));


      // The logical and physical volumes
      G4LogicalVolume* bottomDiscLog = new G4LogicalVolume(bottomDiscSolid,
                                                           bottomDiscMaterial,prefix+"bottom_disc_log");
      SetColor(table,"bottom_disc_colour",bottomDiscLog);
      G4ThreeVector bottomDiscPosition(acrylicPosition.x(),acrylicPosition.y(),
                                       acrylicPosition.z()-acrylicHeight/2.-bottomDiscThickness/2.);
      G4Transform3D bottomDiscTransform(*noRotation,bottomDiscPosition);
      G4PVPlacementWithCheck(bottomDiscTransform,bottomDiscLog,
                             prefix+"bottom_disc_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);


      //place in the electronics just a disc for now
      G4VSolid* electronicsSolid = new G4Tubs (prefix+"electronics_solid",0,electronicsRadius,
                                               electronicsThickness/2.,0.0,CLHEP::twopi);
      // The logical and physical volumes
      G4LogicalVolume* electronicsLog = new G4LogicalVolume(electronicsSolid,
                                                            electronicsMaterial,prefix+"electronics_log");
      SetColor(table,"electronics_colour",electronicsLog);
      G4ThreeVector electronicsPosition(acrylicPosition.x(),acrylicPosition.y(),
                                        acrylicPosition.z()-acrylicHeight/2.+acrylicLEDHeight);
      G4Transform3D electronicsTransform(*noRotation,electronicsPosition);
      G4PVPlacementWithCheck(electronicsTransform,electronicsLog,
                             prefix+"electronics_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //fill the spaces with air first the top part of ufo
      G4VSolid* airSolid1 = new G4Tubs (prefix+"air_solid1",0,acrylicInnerRad,
                                        acrylicHeight/2.,0.0,CLHEP::twopi);
      G4VSolid* airSolid2 = new G4Tubs (prefix+"air_solid2",0,capSpaceRadius,
                                        capSpaceThickness/4.-.318,0.0,CLHEP::twopi);


      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volumes
      G4VSolid* airSolid = new G4UnionSolid(prefix+"air_solid",
                                            airSolid1,airSolid2,noRotation,
                                            G4ThreeVector(0.,0.,acrylicHeight/2.+bottomCupGap));
      airSolid = new G4SubtractionSolid(prefix+"air_solid",
                                        airSolid,electronicsSolid,noRotation,
                                        G4ThreeVector(electronicsPosition.x(),electronicsPosition.y(),
                                                      -acrylicHeight/2.+acrylicLEDHeight));


      // The logical and physical volumes
      G4LogicalVolume* airLog = new G4LogicalVolume(airSolid,
                                                    airMaterial,prefix+"air_log");
      SetColor(table,"air_colour",airLog);
      G4ThreeVector airPosition(acrylicPosition.x(),acrylicPosition.y(),
                                acrylicPosition.z());
      G4Transform3D airTransform(*noRotation,airPosition);
      G4PVPlacementWithCheck(airTransform,airLog,
                             prefix+"air_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //second air space in the bottom cup
      double bottomCupBottomInnerHeight = bottomCupHeight-bottomCupTopHeight-bottomCupMidHeight;
      G4VSolid* air2Solid1 = new G4Tubs (prefix+"air2_solid1",0,bottomCupMidInnerRadius,
                                         bottomCupMidHeight/2.,0.0,CLHEP::twopi);
      G4VSolid* air2Solid2 = new G4Tubs (prefix+"air2_solid2",0,bottomCupBotInnerRadius,
                                         (bottomCupHeight-bottomCupTopHeight-bottomCupMidHeight)/2.,0.0,CLHEP::twopi);

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volumes
      G4VSolid* air2Solid = new G4UnionSolid(prefix+"air2_solid",
                                             air2Solid1,air2Solid2,noRotation,
                                             G4ThreeVector(0.,0.,(-bottomCupHeight+bottomCupBottomInnerHeight/2.+bottomCupGap)/2));//join the air together


      // The logical and physical volumes
      G4LogicalVolume* air2Log = new G4LogicalVolume(air2Solid,
                                                     airMaterial,prefix+"air2_log");
      SetColor(table,"air_colour",air2Log);

      G4ThreeVector air2Position(acrylicPosition.x(),acrylicPosition.y(),
                                 acrylicPosition.z()-acrylicHeight/2.-(bottomCupHeight-bottomCupMidHeight-bottomDiscThickness+bottomCupGap/2.)/2.);
      G4Transform3D air2Transform(*noRotation,air2Position);
      G4PVPlacementWithCheck(air2Transform,air2Log,
                             prefix+"air2_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);
    }
    catch(DBNotFoundError &e) {
      Log::Die("GeoUFOFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field + ".");
    };
  }
} // namespace RAT
