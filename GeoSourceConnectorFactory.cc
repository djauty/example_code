////////////////////////////////////////////////////////////////////////
// Last svn revision: $Id$
////////////////////////////////////////////////////////////////////////

#include <RAT/GeoSourceConnectorFactory.hh>

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
  void GeoSourceConnectorFactory::SetColor(DBLinkPtr table, G4String colourName, G4LogicalVolume *logicalVolume)
  {
    // Set the color of a logical volume

    G4VisAttributes *vis = new G4VisAttributes();
    try {
      const std::vector<double> &color = table->GetDArray(colourName);
      Log::Assert(color.size() == 3 || color.size() == 4, "GeoSourceConnectorFactory: Colour entry " + colourName + " does not have 3 (RGB) or 4 (RGBA) components");
      if(color.size() == 3) // RGB
        vis->SetColour(G4Colour(color[0], color[1], color[2]));
      else if(color.size() == 4) // RGBA
        vis->SetColour(G4Colour(color[0], color[1], color[2], color[3]));
    }
    catch(DBNotFoundError &e){
      Log::Die("GeoSourceConnectorFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field +".");
    };

    logicalVolume->SetVisAttributes(vis);
  } // SetColour

  std::vector<double> GeoSourceConnectorFactory::MultiplyVectorByUnit(std::vector<double> v, const double unit) {
    transform(v.begin(),v.end(),v.begin(),bind2nd(std::multiplies<double>(),unit));
    return v;
  } // MultiplyVectorByUnit

  G4PVPlacement* GeoSourceConnectorFactory::G4PVPlacementWithCheck(
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
                "GeoSourceConnectorFactory: Overlap detected when placing volume " +
                pName + ". See log for details.");
    return placement;
  } // G4PVPlacementWithCheck


  void GeoSourceConnectorFactory::Construct(DBLinkPtr table,
                                            const bool checkOverlaps)
  {
    // Define a default rotation matrix that does not rotate, and other constants
    G4RotationMatrix *noRotation = new G4RotationMatrix();
    const bool pMany = false;
    const int pCopyNo = 0;

    // Get a link to the database storing the source PMT geometry information
    DBLinkPtr sourceConnectorTable = DB::Get()->GetLink("SourceConnector","sourceConnector");

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
                  "GeoSourceConnectorFactory: Unable to find mother volume '" +
                  motherName + "' for '" + index + "'.");

      // Get the position of the centre of the connector and build the volume relative to it
      std::vector<double> const &pos =
        MultiplyVectorByUnit(table->GetDArray("sample_position"),CLHEP::mm);
      Log::Assert(pos.size() == 3,"GeoSourceConnectorFactory: sample_position does not have three components.");
      G4ThreeVector samplePosition(pos[0], pos[1], pos[2]);
      // =====================================
      // Read all parameters from the database
      // =====================================

      // Container parameters
      const double quickConnectRadius = table->GetD("quick_connect_radius") * CLHEP::mm;
      const double quickConnectInnerRadius = table->GetD("quick_connect_inner_radius") * CLHEP::mm;
      const double quickConnectHeight = table->GetD("quick_connect_height") * CLHEP::mm;
      const double quickConnectPlateThickness = table->GetD("quick_connect_plate_thickness") * CLHEP::mm;


      G4Material* const quickConnectMaterial =
        G4Material::GetMaterial(table->GetS("quick_connect_material"));
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

      G4VSolid* containerSolid1 = new G4Tubs(prefix+"container_solid1",quickConnectInnerRadius,
                                             quickConnectRadius,quickConnectHeight/2.0,0.0,CLHEP::twopi);//Quick Connect walls
      G4VSolid* containerSolid2 = new G4Tubs(prefix+"container_solid2",0,
                                             quickConnectInnerRadius,quickConnectPlateThickness/2.0,0.0,CLHEP::twopi);//metal plate

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volume

      G4VSolid* connectorSolid = new G4UnionSolid(prefix+"connector_solid",
                                                  containerSolid1,containerSolid2,noRotation,
                                                  G4ThreeVector(0.,0.,0.));


      // The logical and physical volumes
      G4LogicalVolume* connectorLog = new G4LogicalVolume(connectorSolid,
                                                          quickConnectMaterial,prefix+"connector_log");
      SetColor(table,"quick_connect_colour",connectorLog);
      G4ThreeVector connectorPosition(samplePosition.x(),samplePosition.y(),samplePosition.z());
      G4Transform3D connectorTransform(*noRotation,connectorPosition);
      G4PVPlacementWithCheck(connectorTransform,connectorLog,
                             prefix+"connector_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

      //fill the spaces with air
      G4VSolid* airSolid = new G4Tubs (prefix+"air_solid",0,quickConnectInnerRadius,
                                       quickConnectHeight/2.,0.0,CLHEP::twopi);

      // Now add/subtract volumes to make the container, noting that the first
      // volume specified remains the reference for each subsequent volumes
      airSolid = new G4SubtractionSolid(prefix+"air_solid",
                                        airSolid,containerSolid2,noRotation,
                                        G4ThreeVector(0,0,0));

      // The logical and physical volumes
      G4LogicalVolume* airLog = new G4LogicalVolume(airSolid,
                                                    airMaterial,prefix+"air_log");
      SetColor(table,"air_colour",airLog);
      G4ThreeVector airPosition(connectorPosition.x(),connectorPosition.y(),connectorPosition.z());
      G4Transform3D airTransform(*noRotation,airPosition);
      G4PVPlacementWithCheck(airTransform,airLog,
                             prefix+"air_phys",motherLog,pMany,pCopyNo,
                             pSurfChk);

    }
    catch(DBNotFoundError &e) {
      Log::Die("GeoSourceConnectorFactory: DBNotFoundError. Table " + e.table + ", index " + e.index + ", field " + e.field + ".");
    };
  }
} // namespace RAT
