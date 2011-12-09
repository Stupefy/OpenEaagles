//------------------------------------------------------------------------------
// Classes:
//    Dis::NetIO  -- DIS Network I/O handler class
//    Dis::Nib    -- DIS Network Interface Block class
//    Dis::Ntm    -- DIS Type Mapper class
//------------------------------------------------------------------------------
#ifndef __Eaagles_Network_Dis_NetIO_H__
#define __Eaagles_Network_Dis_NetIO_H__

#include "openeaagles/simulation/NetIO.h"

namespace Eaagles {
   namespace Basic { class NetHandler; }
   namespace Simulation { class Iff; class RfSensor; }

namespace Network {

namespace Dis {
   class Nib;
   class Ntm;
   class EmissionPduHandler;

   struct EeFundamentalParameterData;
   struct EmitterBeamData;
   struct EmitterSystem;
   struct EmissionSystem;
   struct FundamentalOpData;
   struct PDUHeader;
   struct TrackJamTargets;

   struct DetonationPDU;
   struct ElectromagneticEmissionPDU;
   struct EntityStatePDU;
   struct FirePDU;
   struct SignalPDU;
   struct TransmitterPDU;
   struct DataQueryPDU;
   struct DataPDU;
   struct CommentPDU;
   struct StartPDU;
   struct StopPDU;
   struct AcknowledgePDU;
   struct ActionRequestPDU;
   struct ActionRequestPDU_R;
   struct ActionResponsePDU_R;


//==============================================================================
// Class: Dis::NetIO
// Description: Distributed-Interactive-Simulation (DIS) protocol manager.
//
// Slots:
//    netInput    <Basic::NetHandler>     ! Network input handler
//    netOutput   <Basic::NetHandler>     ! Network output handler
//
//    version     <Basic::Number>         ! DIS version number [ 0 .. 6 ] (IST-CF-03-01, May 5, 2003)
//                                        !   0 => Other
//                                        !   1 => DIS PDU version 1.0 (May 92)
//                                        !   2 => IEEE 1278-1993
//                                        !   3 => DIS PDU version 2.0 - third draft (May 93)
//                                        !   4 => DIS PDU version 2.0 - fourth draft (revised) March 16, 1994
//                                        !   5 => IEEE 1278.1-1995
//                                        !   6 => IEEE 1278.1A-1998
//                                        !   7 => IEEE 1278.1-200X
//
//    siteID         <Basic::Number>      ! Site Identification
//    applicationID  <Basic::Number>      ! Application Identification
//    exerciseID     <Basic::Number>      ! Exercise Identification
//
//    maxTimeDR   <Basic::Time>           ! Max DR time (default: 5 seconds)
//                <Basic::PairStream>     ! List of max DR times by kinds and domains (see note #4)
//
//    maxPositionError <Basic::Distance>  ! Max DR position error (default: 3 meters)
//                   <Basic::PairStream> ! List of max DR position errors by kinds and domains (see note #4)
//
//    maxOrientationError <Basic::Angle>  ! Max DR anglular error (default: 3 degrees)
//                    <Basic::PairStream> ! List of max DR anglular errors by kinds and domains (see note #4)
//
//    maxAge         <Basic::Time>        ! Max age (without update) (default: 12.5 seconds)
//                   <Basic::PairStream>  ! List of max ages (without update) by kinds and domains (see note #4)
//
//    maxEntityRange <Basic::Distance>    ! Max entity range, or zero for no max range (default: 0 -- no range filtering)
//                   <Basic::PairStream>  ! List of max entity ranges by kinds and domains (see note #4)
//
//    EmissionPduHandlers <Basic::PairStream> ! List of Electromagnetic-Emission PDU handlers
//
//
// Notes:
//    1) NetIO creates its own federate name based on the site and application numbers
//       using makeFederateName().  (e.g., site = 10 and  app = 143 gives the federate name "S10A143")
//
//    2) NetIO creates its own federation name based on the exercise number
//       using makeFederationName().  (e.g., exercise = 13 gives the federation name "E13")
//
//    3) findDisNib() searches the same input and output lists that are maintained by
//       NetIO, which are in order of player ID and the federate name.  Since our DIS
//       federate names are generated by site and app IDs, the lists are seen by DIS as
//       being order by player ID, site ID and app ID.
//
//    4) For the slots maxTimeDR, maxPositionError, maxOrientationError, maxAge and
//       maxEntityRange, if the slot type is Basic::Time, Basic::Angle or Basic::Distance then that
//       parameter is set for entity types of all kinds and domains.  If a pair stream
//       is given then individual entity kind/domain parameters can be set.  To set the
//       parameters for individual entity kind/domain types, the slot name must have
//       the format:  Kn or KnDm where n and m are the Kind and Domain numbers.
//       Examples --
//          maxTimeDR: { K5: ( Seconds 10.0 )  K1D11: ( Seconds 5.0 ) }
//             K5 will set the parameter for all domains of kinds #5
//             K1D11 will set the parameter for kind #1, domain #11
//
//    5) Setting the 'maxEntityRange' slot to zero(0) for an entity kind/demain
//       will filter out all entities of that kind/domain type.
//
//    6) For outgoing emission PDUs, the list of EmissionPduHandlers are matched
//       with RfSensors using the RfSensor::getTypeId().  That is, the type id
//       of the sensor is matched with the type id of the EmissionPduHandler's
//       type id.  For incoming emission PDUs, the "emitter name" from the PDU
//       is matched with the EmissionPduHandler's "emitterName" value.
//
//==============================================================================
class NetIO : public Simulation::NetIO
{
    DECLARE_SUBCLASS(NetIO,Simulation::NetIO)

public:
   // Max PDU buffer size
   enum { MAX_PDU_SIZE = 1536 };

   // Standard (IST-CF-03-01, May 5, 2003) entity type "kind" codes [ 0 .. 9 ]
   enum EntityTypeKindEnum {
      KIND_OTHER, KIND_PLATFORM, KIND_MUNITION, KIND_LIFEFORM,
      KIND_ENVIRONMENTAL, KIND_CULTURAL_FEATURE, KIND_SUPPLY, KIND_RADIO,
      KIND_EXPENDABLE, KIND_SENSOR_EMITTER, NUM_ENTITY_KINDS
   };

   // Standard (IST-CF-03-01, May 5, 2003) "platform domain" codes [ 0 .. 5 ]
   enum PlatformDomainEnum {
      PLATFORM_DOMAIN_OTHER, PLATFORM_DOMAIN_LAND, PLATFORM_DOMAIN_AIR, PLATFORM_DOMAIN_SURFACE,
      PLATFORM_DOMAIN_SUBSURFACE, PLATFORM_DOMAIN_SPACE
   };

   // Standard (IST-CF-03-01, May 5, 2003) "munition domain" codes [ 0 .. 11 ]
   enum MunitionDomainEnum {
      MUNITION_DOMAIN_OTHER, MUNITION_DOMAIN_ANTI_AIR, MUNITION_DOMAIN_ANTI_ARMOR, MUNITION_DOMAIN_ANTI_GUIDED_MUNITION,
      MUNITION_DOMAIN_ANTIRADAR, MUNITION_DOMAIN_ANTISATELLITE, MUNITION_DOMAIN_ANTISHIP, MUNITION_DOMAIN_ANTISUBMARINE,
      MUNITION_DOMAIN_ANTIPERSONNEL, MUNITION_DOMAIN_BATTLEFIELD_SUPPORT, MUNITION_DOMAIN_STRATEGIC, MUNITION_DOMAIN_TACTICAL
   };

   // Larges number of domains in any kind (IST-CF-03-01, May 5, 2003)
   enum { MAX_ENTITY_DOMAINS = MUNITION_DOMAIN_TACTICAL };

   // Standard (IST-CF-03-01, May 5, 2003) "country" codes
   enum EntityTypeCountryEnum {
      COUNTRY_OTHER = 0, COUNTRY_FRANCE = 71, COUNTRY_CIS = 222, COUNTRY_UK = 224, COUNTRY_USA = 225,
   };

    // Standard (IST-CF-03-01, May 5, 2003) "force" codes [ 0 .. 3 ]
    enum ForceEnum {
        OTHER_FORCE,   FRIENDLY_FORCE,   OPPOSING_FORCE,   NEUTRAL_FORCE
    };

   // Standard (IST-CF-03-01, May 5, 2003) "DIS Protocol Version" codes [ 0 .. 6 ]
   enum {
         VERSION_OTHER,    // Other
         VERSION_100,      // DIS PDU version 1.0 (May 92)
         VERSION_1278,     // IEEE 1278-1993
         VERSION_203,      // DIS PDU version 2.0 - third draft (May 93)
         VERSION_204,      // DIS PDU version 2.0 - fourth draft (revised) March 16, 1994
         VERSION_1278_1,   // IEEE 1278.1-1995
         VERSION_1278_1A,  // IEEE 1278.1A-1998
         VERSION_7,        // IEEE P1278.1/D15
         VERSION_MAX,      // Max version numbers
    };

   // SISO-REF-010-2006; 12th May, 2006; Section 3.2 PDU Type
   enum {
      PDU_OTHER=0,                  PDU_ENTITY_STATE=1,           PDU_FIRE=2,
      PDU_DETONATION=3,             PDU_COLLISION=4,              PDU_SERVICE_REQUEST=5,
      PDU_RESUPPLY_OFFER=6,         PDU_RESUPPLY_RECEIVED=7,      PDU_RESUPPLY_CANCEL=8,
      PDU_REPAIR_COMPLETE=9,        PDU_REPAIR_RESPONSE=10,       PDU_CREATE_ENTITY=11,
      PDU_REMOVE_ENTITY=12,         PDU_START_RESUME=13,          PDU_STOP_FREEZE=14,
      PDU_ACKNOWLEDGE=15,           PDU_ACTION_REQUEST=16,        PDU_ACTION_RESPONSE=17,
      PDU_DATA_QUERY=18,            PDU_SET_DATA=19,              PDU_DATA=20,
      PDU_EVENT_REPORT=21,          PDU_COMMENT=22,               PDU_ELECTROMAGNETIC_EMISSION=23,
      PDU_DESIGNATOR=24,            PDU_TRANSMITTER=25,           PDU_SIGNAL=26,
      PDU_RECEIVER=27,              PDU_IFF_ATC_NAVAIDS=28,       PDU_UNDERWATER_ACOUSTIC=29,
      PDU_SUPPLEMENTAL_EMISSION=30, PDU_INTERCOM_SIGNAL=31,       PDU_INTERCOM_CONTROL=32,
      PDU_AGGREGATE_STATE=33,       PDU_IS_GROUP_OF=34,           PDU_TRANSFER_CONTROL=35,
      PDU_IS_PART_OF=36,            PDU_MINEFIELD_STATE=37,       PDU_MINEFIELD_QUERY=38,
      PDU_MINEFIELD_DATA=39,        PDU_MINEFIELD_RESPONSE_NAK=40, PDU_ENVIRONMENTAL_PROCESS=41,
      PDU_GRIDDED_DATA=42,          PDU_POINT_OBJECT_STATE=43,    PDU_LINEAR_OBJECT_STATE=44,
      PDU_AREAL_OBJECT_STATE=45,    PDU_TSPI=46,                  PDU_APPEARANCE=47,
      PDU_ARTICULATED_PARTS=48,     PDU_LE_FIRE=49,               PDU_LE_DETONATION=50,
      PDU_CREATE_ENTITY_R=51,       PDU_REMOVE_ENTITY_R=52,       PDU_START_RESUME_R=53,
      PDU_STOP_FREEZE_R=54,         PDU_ACKNOWLEDGE_R=55,         PDU_ACTION_REQUEST_R=56,
      PDU_ACTION_RESPONSE_R=57,     PDU_DATA_QUERY_R=58,          PDU_SET_DATA_R=59,
      PDU_DATA_R=60,                PDU_EVENT_REPORT_R=61,        PDU_COMMENT_R=62,
      PDU_RECORD_R=63,              PDU_SET_RECORD_R=64,          PDU_RECORD_QUERY_R=65,
      PDU_COLLISION_ELASTIC=66,     PDU_ENTITY_STATE_UPDATE=67,

      PDU_ANNOUNCE_OBJECT=129,      PDU_DELETE_OBJECT = 130,
      PDU_DESCRIBE_APPLICATION=131, PDU_DESCRIBE_EVENT = 132,
      PDU_DESCRIBE_OBJECT=133,      PDU_REQUEST_EVENT = 134,
      PDU_REQUEST_OBJECT=135
   };

   // Standard (IST-CF-03-01, May 5, 2003) PDU Family
   enum {
      PDU_FAMILY_OTHER,                // other
      PDU_FAMILY_ENTITY_INFO,          // Entity Information/Interaction
      PDU_FAMILY_WARFARE,              // Warfare
      PDU_FAMILY_LOGISTICS,            // Logistics
      PDU_FAMILY_RADIO_COMM,           // Radio Communication
      PDU_FAMILY_SIMULATION_MAN,       // Simulation Management
      PDU_FAMILY_DIS_EMISSION_REG,     // Distributed Emission Regeneration
      PDU_FAMILY_ENTITY_MAN,           // Entity Management
      PDU_FAMILY_MINEFIELD,            // Minefield
      PDU_FAMILY_SYNTHETIC_ENV,        // Synthetic Environment
      PDU_FAMILY_SIMULATION_MAN_REL,   // Simulation Management with Reliability
      PDU_FAMILY_LIVE_ENTITY,          // Live Entity
      PDU_FAMILY_NON_REAL_TIME,        // Non-Real Time
      PDU_FAMILY_EXPERIMENTAL = 129,   // Experimental - Computer Generated Forces
   };


public:
   NetIO();

   // Network Identifications
   unsigned short getSiteID() const                        { return siteID; }
   unsigned short getApplicationID() const                 { return appID; }
   unsigned char getExerciseID() const                     { return exerciseID; }

   // Sends a packet (PDU) to the network
   bool sendData(const char* const packet, const int size);

   // Receives a packet (PDU) from the network
   int recvData(char* const packet, const int maxSize);

   unsigned int timeStamp();                                                  // Gets the current timestamp
   unsigned int makeTimeStamp(const LCreal ctime, const bool absolute);       // Make a PDU time stamp

   bool isVersion(const unsigned char v) const    { return (v == version); }  // True if versions match
   unsigned char getVersion() const               { return version; }         // Returns the current version number
   virtual bool setVersion(const unsigned char v);                            // Sets the operating version number

   // Emisison PDU handler
   const EmissionPduHandler* findEmissionPduHandler(const Simulation::RfSensor* const msg);
   const EmissionPduHandler* findEmissionPduHandler(const EmissionSystem* const msg);

   // Generate a federate name from the site and application numbers:
   //  "SnnAmm" -- where nn and mm are the site and app numbers.
   static bool makeFederateName(char* const fedName, const unsigned int len, const unsigned short site, const unsigned short app);

   // Parse federate name for the site and application numbers
   //  (We're expecting "SnnAmm" where nn and mm are the site and app numbers.)
   static bool parseFederateName(unsigned short* const site, unsigned short* const app, const char* const fedName);

   // Generate a federation name from the exercise numbers:
   //  "Ennn" -- where nnn is the exercise number, which must be greater than zero
   static bool makeFederationName(char* const fedName, const unsigned int len, const unsigned short exercise);

   // Parse federation name for the exercise number
   //  (We're expecting "Ennn" where nnn is the exercise.)
   static bool parseFederationName(unsigned short* const exercise, const char* const fedName);

   // Finds the Nib for 'ioType' by player, site and app IDs
   virtual Nib* findDisNib(const unsigned short playerId, const unsigned short siteId, const unsigned short appId, const IoType ioType);

   // Finds the Ntm by DIS entity type codes
   virtual const Ntm* findNtmByTypeCodes(
         const unsigned char  kind,
         const unsigned char  domain,
         const unsigned short countryCode,
         const unsigned char  category,
         const unsigned char  subcategory = 0,
         const unsigned char  specific = 0,
         const unsigned char  extra = 0
      ) const;

   // NetIO Interface
   virtual LCreal getMaxEntityRange(const Simulation::Nib* const nib) const;
   virtual LCreal getMaxEntityRangeSquared(const Simulation::Nib* const nib) const;
   virtual LCreal getMaxTimeDR(const Simulation::Nib* const nib) const;
   virtual LCreal getMaxPositionErr(const Simulation::Nib* const nib) const;
   virtual LCreal getMaxOrientationErr(const Simulation::Nib* const nib) const;
   virtual LCreal getMaxAge(const Simulation::Nib* const nib) const;
   virtual Simulation::Nib* createNewOutputNib(Simulation::Player* const player);

   // DIS v7 additions
   virtual LCreal getHbtPduEe() const;
   virtual LCreal getHbtTimeoutMplier() const;
   virtual LCreal getEeAzThrsh() const;
   virtual LCreal getEeElThrsh() const;

   virtual LCreal getEeErpThrsh() const;
   virtual LCreal getEeFreqThrsh() const;
   virtual LCreal getEeFrngThrsh() const;
   virtual LCreal getEePrfThrsh() const;
   virtual LCreal getEePwThrsh() const;

protected:
   virtual void processEntityStatePDU(const EntityStatePDU* const pdu);
   virtual void processFirePDU(const FirePDU* const pdu);
   virtual void processDetonationPDU(const DetonationPDU* const pdu);
   virtual void processElectromagneticEmissionPDU(const ElectromagneticEmissionPDU* const pdu);
   virtual bool processSignalPDU(const SignalPDU* const pdu);
   virtual bool processTransmitterPDU(const TransmitterPDU* const pdu);
   virtual bool processDataQueryPDU(const DataQueryPDU* const pdu);
   virtual bool processDataPDU(const DataPDU* const pdu);
   virtual bool processCommentPDU(const CommentPDU* const pdu);
   virtual bool processStartPDU(const StartPDU* const pdu);
   virtual bool processStopPDU(const StopPDU* const pdu);
   virtual bool processAcknowledgePDU(const AcknowledgePDU* const pdu);
   virtual bool processActionRequestPDU(const ActionRequestPDU* const pdu);
   virtual bool processActionRequestPDU_R(const ActionRequestPDU_R* const pdu);
   virtual bool processActionResponsePDU_R(const ActionResponsePDU_R* const pdu);

   // User defined function to process unknown PDUs (PDU bytes are still in network order)
   virtual bool processUserPDU(const PDUHeader* const pdu);

   virtual void clearEmissionPduHandlers();
   virtual void addEmissionPduHandler(const EmissionPduHandler* const item);
   virtual void defineFederateName();
   virtual void defineFederationName();

   // Set functions
   virtual bool setSiteID(const unsigned short v);          // Sets the network's site ID
   virtual bool setApplicationID(const unsigned short v);   // Sets the network's application ID
   virtual bool setExerciseID(const unsigned char v);       // Sets the network's exercise ID

   virtual bool setSlotNetInput(Basic::NetHandler* const msg);               // Network input handler
   virtual bool setSlotNetOutput(Basic::NetHandler* const msg);              // Network output handler
   virtual bool setSlotVersion(const Basic::Number* const num);              // DIS version
   virtual bool setSlotMaxTimeDR(const Basic::PairStream* const msg);        // Sets the max DR time(s) for selected entity types
   virtual bool setSlotMaxTimeDR(const Basic::Time* const msg);              // Sets the max DR time(s) for all entity types
   virtual bool setSlotMaxPositionErr(const Basic::PairStream* const msg);   // Sets the max positional error(s) for selected entity types
   virtual bool setSlotMaxPositionErr(const Basic::Distance* const msg);     // Sets the max positional error(s) for all entity types
   virtual bool setSlotMaxOrientationErr(const Basic::PairStream* const msg); // Sets the max orientation error(s) for selected entity types
   virtual bool setSlotMaxOrientationErr(const Basic::Angle* const msg);     // Sets the max orientation error(s) for all entity types
   virtual bool setSlotMaxAge(const Basic::PairStream* const msg);           // Sets the max age(s) for selected entity types
   virtual bool setSlotMaxAge(const Basic::Time* const msg);                 // Sets the max age(s) for all entity types
   virtual bool setSlotMaxEntityRange(const Basic::PairStream* const msg);   // Sets the max entity range(s) for selected entity types
   virtual bool setSlotMaxEntityRange(const Basic::Distance* const msg);     // Sets the max entity range(s) for all entity types
   virtual bool setSlotEmissionPduHandlers(Basic::PairStream* const msg);    // Sets the list of Electromagnetic Emission PDU handlers
   virtual bool setSlotSiteID(const Basic::Number* const num);               // Sets Site ID
   virtual bool setSlotApplicationID(const Basic::Number* const num);        // Sets Application ID
   virtual bool setSlotExerciseID(const Basic::Number* const num);           // Sets Exercise ID

   virtual bool slot2KD(const char* const slotname, unsigned char* const k, unsigned char* const d);
   virtual bool setMaxTimeDR(const LCreal v, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxTimeDR(const Basic::Time* const p, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxPositionErr(const LCreal v, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxPositionErr(const Basic::Distance* const p, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxOrientationErr(const LCreal v, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxOrientationErr(const Basic::Angle* const p, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxAge(const LCreal v, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxAge(const Basic::Time* const p, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxEntityRange(const LCreal v, const unsigned char kind, const unsigned char domain);
   virtual bool setMaxEntityRange(const Basic::Distance* const p, const unsigned char kind, const unsigned char domain);

   // NetIO Interface (overriding these slots!)
   virtual bool setSlotFederateName(const Basic::String* const msg);         // Sets our federate name
   virtual bool setSlotFederationName(const Basic::String* const msg);       // Sets our federation name

   // NetIO Interface
   virtual bool initNetwork();             // Initialize the network
   virtual void netInputHander();          // Network input handler
   virtual void processInputList();        // Update players/systems from the Input-list
   virtual Simulation::Nib* nibFactory(const Simulation::NetIO::IoType ioType);  // Create a new Nib
   virtual Simulation::NetIO::NtmInputNode* rootNtmInputNodeFactory() const;
   virtual void testOutputEntityTypes(const unsigned int);   // Test quick lookup of outgoing entity types
   virtual void testInputEntityTypes(const unsigned int);    // Test quick lookup of incoming entity types

private:

    SPtr<Basic::NetHandler>   netInput;          // Input network handler
    SPtr<Basic::NetHandler>   netOutput;         // Output network handler
    unsigned char        version;           // Version number [ 0 .. 6 ]

   // Network Model IDs
   unsigned short    siteID;           // Site ID
   unsigned short    appID;            // Application ID
   unsigned char     exerciseID;       // Exercise ID

   // Distance filter by entity kind/domain
   LCreal  maxEntityRange[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];     // Max range from ownship           (meters)
   LCreal  maxEntityRange2[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];   // Max range squared from ownship   (meters^2)

   // Dead Reckoning (DR) parameters by entity kind/domain
   LCreal  maxTimeDR[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];          // Maximum DR time                  (seconds)
   LCreal  maxPositionErr[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];     // Maximum position error           (meters)
   LCreal  maxOrientationErr[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];  // Maximum orientation error        (radians)
   LCreal  maxAge[NUM_ENTITY_KINDS][MAX_ENTITY_DOMAINS];             // Maximum age of networked players (seconds)

   static const unsigned int MAX_EMISSION_HANDLERS = 500;            // Max table size

   // Table of pointers to emission PDU handlers; EmissionPduHandler objects
   const EmissionPduHandler* emissionHandlers[MAX_EMISSION_HANDLERS];

   // Number of iemission PDU handlers in the table, 'emissionHandlers'
   unsigned int   nEmissionHandlers;
};


} // End Dis namespace
} // End Network namespace
} // End Eaagles namespace

#endif
