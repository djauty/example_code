////////////////////////////////////////////////////////////////////////
// \class RAT::GeoTaggedSourceFactory
//
// \brief Geometry for the tagged calibration source
//
// \author Logan Sibley <lsibley@ualberta.ca>
//
// REVISION HISTORY:\n
//     07/11/2013 : L. Sibley - First version. \n
//     16/01/2016 : D J Auty - updated geometry to current source design
//     24/03/2016 : D J Auty converted to a tagged source not just Co60
//     14/12/2017 : D J Auty split out the Source connector
//
//
// \detail Construct the tagged source with stem
//
//         To load this geometry in the simulation, use
//
//             /rat/db/load geo/calibTaggedSource.geo
//
//         Should be used with the source connector and ufo, use
//             /rat/db/load geo/calib/UFO.geo
//             /rat/db/load geo/calib/SourceConnector.geo
//
//         This geometry is intended to be used to study calibration of SNO+
//         using a tagged source, which emits two gamma rays and a beta.
//
//         The co60source/sc46source generator should be used, as it automatically sets
//         the position of the source and uses the appropriate generator for
//         the MC events.
//
//             /generator/add co60source
//             /generator/add sc46source
//
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_GeoTaggedSourceFactory__
#define __RAT_GeoTaggedSourceFactory__

#include <RAT/GeoFactory.hh>
#include <G4PVPlacement.hh>

namespace RAT
{

  class GeoTaggedSourceFactory : public GeoFactory
  {
  public:
    GeoTaggedSourceFactory() : GeoFactory("TaggedSource") {};
    virtual ~GeoTaggedSourceFactory() { };
    //virtual G4VPhysicalVolume* Construct(DBLinkPtr table);
    virtual void Construct(DBLinkPtr table, const bool checkOverlaps);
  private:
    void SetColor(DBLinkPtr table, G4String colorName,
                  G4LogicalVolume *logicalVolume);
    std::vector<double> MultiplyVectorByUnit(std::vector<double> v,
                                             const double unit);
    G4PVPlacement* G4PVPlacementWithCheck(G4Transform3D &Transform3D,
                                          G4LogicalVolume *pCurrentLogical,
                                          const G4String &pName,
                                          G4LogicalVolume *pMotherLogical,
                                          G4bool pMany,
                                          G4int pCopyNo,
                                          G4bool pSurfChk = false);
  };

} // namespace RAT

#endif
