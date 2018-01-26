////////////////////////////////////////////////////////////////////////
// \class RAT::GeoSourceConnectorFactory
//
// \brief Geometry for the source connector
//
// \author David Auty <auty@ualberta.ca>
//
// REVISION HISTORY:\n
//     06/10/2017 : D J Auty - First version. \n
//
//
// \detail Construct the source connetor for the calibratiuon sources
//
//         To load this geometry in the simulation, use
//
//             /rat/db/load geo/calib/SourceConector.geo
//
//         This geometry is intended to be used to simulate the source
//              connector attachment which is part of some calibration
//              sources
//
//
//
////////////////////////////////////////////////////////////////////////

#ifndef __RAT_GeoSourceConnectorFactory__
#define __RAT_GeoSourceConnectorFactory__

#include <RAT/GeoFactory.hh>
#include <G4PVPlacement.hh>

namespace RAT
{
  class GeoSourceConnectorFactory : public GeoFactory
  {
  public:
    GeoSourceConnectorFactory() : GeoFactory("SourceConnector") {};
    virtual ~GeoSourceConnectorFactory() { };
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
