/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Marco Miozzo <mmiozzo@cttc.es>
 *          Manuel Requena <manuel.requena@cttc.es> 
 *
 * Modified by: Michele Polese <michele.polese@gmail.com>
 *          Dual Connectivity functionalities
 */

#ifndef LTE_ENB_RRC_H
#define LTE_ENB_RRC_H

#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/traced-callback.h>
#include <ns3/event-id.h>

#include <ns3/lte-enb-cmac-sap.h>
#include <ns3/lte-mac-sap.h>
#include <ns3/ff-mac-sched-sap.h>
#include <ns3/ff-mac-csched-sap.h>
#include <ns3/lte-pdcp-sap.h>
#include <ns3/epc-x2-sap.h>
#include <ns3/epc-enb-s1-sap.h>
#include <ns3/lte-handover-management-sap.h>
#include <ns3/lte-enb-cphy-sap.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/lte-anr-sap.h>
#include <ns3/lte-ffr-rrc-sap.h>
#include <ns3/lte-rlc.h>
#include <ns3/lte-pdcp.h>
#include <ns3/lte-rlc-am.h>

#include <map>
#include <set>

namespace ns3 {

class LteRadioBearerInfo;
class LteSignalingRadioBearerInfo;
class LteDataRadioBearerInfo;
class RlcBearerInfo;
class LteEnbRrc;
class Packet;

typedef std::map<uint64_t, double> ImsiSinrMap;
typedef std::map<uint16_t, double> CellSinrMap;

/**
 * \ingroup lte
 * Manages all the radio bearer information possessed by the ENB RRC for a
 * single UE.
 */
class UeManager : public Object
{
  friend class LtePdcpSpecificLtePdcpSapUser<UeManager>;

public:


  /**
   * The state of the UeManager at the eNB RRC
   * 
   */
  enum State
  {
    INITIAL_RANDOM_ACCESS = 0,
    CONNECTION_SETUP,
    CONNECTION_REJECTED,
    CONNECTED_NORMALLY,
    CONNECTION_RECONFIGURATION,
    CONNECTION_REESTABLISHMENT,
    HANDOVER_PREPARATION,
    HANDOVER_JOINING,
    HANDOVER_PATH_SWITCH,
    HANDOVER_LEAVING,
    PREPARE_MC_CONNECTION_RECONFIGURATION,
    MC_CONNECTION_RECONFIGURATION,
    NUM_STATES
  };

  UeManager ();
 
  /** 
   * UeManager constructor
   * 
   * \param rrc pointer to the LteEnbRrc holding this UeManager
   * \param rnti RNTI of the UE
   * \param s initial state of the UeManager
   * 
   * \return 
   */
  UeManager (Ptr<LteEnbRrc> rrc, uint16_t rnti, State s);

  virtual ~UeManager (void);

  // inherited from Object
protected:
  virtual void DoInitialize ();
  virtual void DoDispose ();
public: 
  static TypeId GetTypeId (void);

  /** 
   * Set the identifiers of the source eNB for the case where a UE
   * joins the current eNB as part of a handover procedure 
   * 
   * \param sourceCellId 
   * \param sourceX2apId 
   */
  void SetSource (uint16_t sourceCellId, uint16_t sourceX2apId);
  std::pair<uint16_t, uint16_t> GetSource (void);

  /** 
   * Set the IMSI
   * 
   * \param imsi the IMSI
   */
  void SetImsi (uint64_t imsi);

  // set if this is a MC device
  void SetIsMc (bool isMc);
  // know if this is a MC device
  bool GetIsMc (void) const;

  //void SetIsInterRatHoCapable (bool isInterRatHoCapable);

  /** 
   * Setup a new data radio bearer, including both the configuration
   * within the eNB and the necessary RRC signaling with the UE
   * 
   * \param bearer the QoS characteristics of the bearer
   * \param bearerId the EPS bearer identifier
   * \param gtpTeid S1-bearer GTP tunnel endpoint identifier, see 36.423 9.2.1
   * \param transportLayerAddress  IP Address of the SGW, see 36.423 9.2.1
   * \param IAB a boolean that indicates if the bearer is for an IAB device
   * 
   */
  void SetupDataRadioBearer (EpsBearer bearer, uint8_t bearerId, uint32_t gtpTeid, Ipv4Address transportLayerAddress, bool iab);

  /** 
   * Start all configured data radio bearers. It is safe to call this
   * method if any bearer had been already started previously.
   * 
   */
  void RecordDataRadioBearersToBeStarted ();

  /** 
   * Start the data radio bearers that have been previously recorded
   * to be started using RecordDataRadioBearersToBeStarted() 
   * 
   */
  void StartDataRadioBearers ();

  /**
   *
   * Release a given radio bearer
   *
   * \param drbid the data radio bearer id of the bearer to be released
   */
  void ReleaseDataRadioBearer (uint8_t drbid);

  /** 
   * schedule an RRC Connection Reconfiguration procedure with the UE
   * 
   */
  void ScheduleRrcConnectionReconfiguration ();

  /** 
   * Start the handover preparation and send the handover request
   * 
   * \param cellId id of the target cell
   */
  void PrepareHandover (uint16_t cellId);

  /** 
   * take the necessary actions in response to the reception of an X2 HANDOVER REQUEST ACK message
   * 
   * \param params 
   */
  void RecvHandoverRequestAck (EpcX2SapUser::HandoverRequestAckParams params);

  /** 
   * 
   * \return the HandoverPreparationInfo sent by the source eNB to the
   * target eNB in the X2-based handover procedure
   */
  LteRrcSap::RadioResourceConfigDedicated GetRadioResourceConfigForHandoverPreparationInfo ();

  /** 
   * 
   * \return retrieve the data that the target eNB needs to send to the source
   * eNB as the Handover Command in the X2-based handover
   * procedure.
   *
   * \note mobility control info is not expected to be filled in
   * (shall be filled in by the caller). 
   */
  LteRrcSap::RrcConnectionReconfiguration GetRrcConnectionReconfigurationForHandover ();

  /** 
   * Send a data packet over the appropriate Data Radio Bearer
   * 
   * \param bid the corresponding EPS Bearer ID
   * \param p the packet
   */
  void SendData (uint8_t bid, Ptr<Packet> p);

  /** 
   * 
   * \return a list of ERAB-to-be-setup items to be put in a X2 HO REQ message
   */
  std::vector<EpcX2Sap::ErabToBeSetupItem> GetErabList ();

  /** 
   * send the UE CONTEXT RELEASE X2 message to the source eNB, thus
   * successfully terminating an X2 handover procedure 
   * 
   */
  void SendUeContextRelease ();

  void SendRrcConnectionSwitch(bool useMmWaveConnection);

  /** 
   * Take the necessary actions in response to the reception of an X2 HO preparation failure message
   * 
   * \param cellId id of the target cell
   */
  void RecvHandoverPreparationFailure (uint16_t cellId);

  /** 
   * Take the necessary actions in response to the reception of an X2 SN STATUS TRANSFER message
   * 
   * \param params the SN STATUS
   */
  void RecvSnStatusTransfer (EpcX2SapUser::SnStatusTransferParams params);
 
  /** 
   * Take the necessary actions in response to the reception of an X2 UE CONTEXT RELEASE message
   * 
   * \param params the SN STATUS
   */
  void RecvUeContextRelease (EpcX2SapUser::UeContextReleaseParams params); 

  /**
   * Setup a new RLC entity and the X2 related connection
   */
  void RecvRlcSetupRequest (EpcX2SapUser::RlcSetupRequest params);

  /**
   * Ack the setup of the RLC remote entity, start the re-configuration on the UE
   */
  void RecvRlcSetupCompleted (uint8_t drbid);


  // METHODS FORWARDED FROM ENB RRC SAP ///////////////////////////////////////

  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::CompleteSetupUe interface.
  void CompleteSetupUe (LteEnbRrcSapProvider::CompleteSetupUeParameters params);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvRrcConnectionRequest interface.
  void RecvRrcConnectionRequest (LteRrcSap::RrcConnectionRequest msg);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvRrcConnectionSetupCompleted interface.
  void RecvRrcConnectionSetupCompleted (LteRrcSap::RrcConnectionSetupCompleted msg);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvRrcConnectionReconfigurationCompleted interface.
  void RecvRrcConnectionReconfigurationCompleted (LteRrcSap::RrcConnectionReconfigurationCompleted msg);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvRrcConnectionReestablishmentRequest interface.
  void RecvRrcConnectionReestablishmentRequest (LteRrcSap::RrcConnectionReestablishmentRequest msg);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvRrcConnectionReestablishmentComplete interface.
  void RecvRrcConnectionReestablishmentComplete (LteRrcSap::RrcConnectionReestablishmentComplete msg);
  /// Part of the RRC protocol. Implement the LteEnbRrcSapProvider::RecvMeasurementReport interface.
  void RecvMeasurementReport (LteRrcSap::MeasurementReport msg);
  ///
  void RecvRrcSecondaryCellInitialAccessSuccessful(uint16_t mmWaveRnti, uint16_t mmWaveCellId);


  // METHODS FORWARDED FROM ENB CMAC SAP //////////////////////////////////////

  void CmacUeConfigUpdateInd (LteEnbCmacSapUser::UeConfig cmacParams);

  // METHODS FORWARDED FROM ENB PDCP SAP //////////////////////////////////////

  void DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params);

  // IAB
  void NotifyNumIabPerRnti (EpcEnbS1SapUser::NotifyNumIabPerRntiParameters params);


  /** 
   * 
   * \return the RNTI, i.e., an UE identifier that is unique within
   * the cell
   */
  uint16_t GetRnti (void) const;

  /** 
   *
   * \return the IMSI, i.e., a globally unique UE identifier
   */
  uint64_t GetImsi (void) const;

  /** 
   * 
   * \return the SRS Configuration Index
   */
  uint16_t GetSrsConfigurationIndex (void) const;

  /** 
   * Set the SRS configuration index and do the necessary reconfiguration
   * 
   * \param srsConfIndex 
   */
  void SetSrsConfigurationIndex (uint16_t srsConfIndex);

  /** 
   * 
   * \return the current state
   */
  State GetState () const;

  /**
   * Configure PdschConfigDedicated (i.e. P_A value) for UE and start RrcConnectionReconfiguration
   * to inform UE about new PdschConfigDedicated
   *
   * \param pdschConfigDedicated new pdschConfigDedicated (i.e. P_A value) to be set
   */
  void SetPdschConfigDedicated (LteRrcSap::PdschConfigDedicated pdschConfigDedicated);

  /**
   * TracedCallback signature for state transition events.
   *
   * \param [in] imsi
   * \param [in] cellId
   * \param [in] rnti
   * \param [in] oldState
   * \param [in] newState
   */
  typedef void (* StateTracedCallback)
    (uint64_t imsi, uint16_t cellId, uint16_t rnti,
     State oldState, State newState);

  /**
   * TracedCallback signature for imsi, cellId and rnti events.
   *
   * \param [in] imsi
   * \param [in] cellId
   * \param [in] rnti
   */
  typedef void (* ImsiCidRntiTracedCallback)
    (uint64_t imsi, uint16_t cellId, uint16_t rnti);

  // for interRatHandover mode
  void SetFirstConnection();

  // for mc devices, get the m_allMmWaveInOutageAtInitialAccess variable
  bool GetAllMmWaveInOutageAtInitialAccess();
  void SetAllMmWaveInOutageAtInitialAccess(bool param);

  /**
   * This method is called by the eNB RRC to notify the 
   * UeManager that the handover from a MmWave cell to LTE
   * was completed. Then, according to the state of 
   * the UeManager, it will modify the m_mmWaveCellSetupCompleted map
   * of RRC.
   * Used only with InterRat HO mode
   */
  void RecvNotifyLteMmWaveHandoverCompleted();

  /**
   * Notify a mmWave remote UeManager that a switch is expected. 
   * If the switch is to LTE, forward RLC buffers to LTE eNB
   */
  void RecvConnectionSwitchToMmWave (bool useMmWaveConnection, uint8_t drbid);

  /**
   * A SecondaryCellHandoverCompleted is received by the LTE eNB when the
   * handover between 2 mmWave eNBs is completed. The LTE eNB has to update 
   * the information on the secondary cell and send the connection release to the
   * source mmWave eNB.
   */
  void RecvSecondaryCellHandoverCompleted (EpcX2SapUser::SecondaryHandoverCompletedParams params);

private:
  //Lossless HO: merge 2 buffers into 1 with increment order.
  std::vector < LteRlcAm::RetxPdu > MergeBuffers(std::vector < LteRlcAm::RetxPdu > first, std::vector < LteRlcAm::RetxPdu > second);
  /**
   * Forward the content of RLC buffers. For RLC UM and UM LowLat, forward txBuffer.
   * For RLC AM, forward the merge of retx and txed buffers, and txBuffer
   * @param the RLC entity
   * @param the PDCP entity, if the mcLteToMmWaveForwarding is false it can be 0
   * @param the gtpTeid that identifies the X2 connection to be used
   * @param a bool that specifies if this operation involves a classic forwarding (to PDCP of the target eNB)
   * or a LTE to MmWave Dual Connectivity forwarding, which needs the packets to be inserted back in the LTE PDCP
   * (there is no PDCP in the MmWave eNB in DC setup)
   * @param a bool that specifies if this operation involves the forwarding of packets from a secondary mmWave cell
   * to another secondary mmWave cell during a secondary cell HO 
   * @param the bearer id, used only if mcLteToMmWaveForwarding is true
   */
  void ForwardRlcBuffers(Ptr<LteRlc> rlc, Ptr<LtePdcp> pdcp, uint32_t gtpTeid, bool mcLteToMmWaveForwarding, bool mcMmToMmWaveForwarding, uint8_t bid);

  
  bool m_firstConnection;
  bool m_receivedLteMmWaveHandoverCompleted;
  uint16_t m_queuedHandoverRequestCellId;
  /** 
   * Add a new LteDataRadioBearerInfo structure to the UeManager
   * 
   * \param radioBearerInfo 
   * 
   * \return the id of the newly added data radio bearer structure
   */
  uint8_t AddDataRadioBearerInfo (Ptr<LteDataRadioBearerInfo> radioBearerInfo);

  /** 
   * \param drbid the Data Radio Bearer id
   * 
   * \return the corresponding LteDataRadioBearerInfo
   */
  Ptr<LteDataRadioBearerInfo> GetDataRadioBearerInfo (uint8_t drbid);

  /** 
   * remove the LteDataRadioBearerInfo corresponding to a bearer being released
   *
   * \param drbid the Data Radio Bearer id
   */
  void RemoveDataRadioBearerInfo (uint8_t drbid);

  /** 
   * 
   * \return an RrcConnectionReconfiguration struct built based on the
   * current configuration
   */
  LteRrcSap::RrcConnectionReconfiguration BuildRrcConnectionReconfiguration ();

  /** 
   * 
   * \return a RadioResourceConfigDedicated struct built based on the
   * current configuration
   */
  LteRrcSap::RadioResourceConfigDedicated BuildRadioResourceConfigDedicated ();

  /** 
   * 
   * \return a newly allocated identifier for a new RRC transaction
   */
  uint8_t GetNewRrcTransactionIdentifier ();

  /** 
   * \param lcid a Logical Channel Identifier
   * 
   * \return the corresponding Data Radio Bearer Id
   */
  uint8_t Lcid2Drbid (uint8_t lcid);

  /** 
   * \param drbid a Data Radio Bearer Id
   * 
   * \return the corresponding  Logical Channel Identifier
   */
  uint8_t Drbid2Lcid (uint8_t drbid);

  /** 
   * \param lcid a  Logical Channel Identifier
   * 
   * \return the corresponding EPS Bearer Identifier
   */
  uint8_t Lcid2Bid (uint8_t lcid);

  /** 
   * \param bid  an EPS Bearer Identifier
   * 
   * \return the corresponding Logical Channel Identifier
   */
  uint8_t Bid2Lcid (uint8_t bid);

  /** 
   * \param drbid Data Radio Bearer Id
   * 
   * \return the corresponding EPS Bearer Identifier
   */
  uint8_t Drbid2Bid (uint8_t drbid);

  /** 
   * \param bid an EPS Bearer Identifier
   * 
   * \return the corresponding Data Radio Bearer Id
   */
  uint8_t Bid2Drbid (uint8_t bid);

  /** 
   * Switch the UeManager to the given state
   * 
   * \param s the given state
   */
  void SwitchToState (State s);

  uint8_t m_lastAllocatedDrbid;

  /**
   * The `DataRadioBearerMap` attribute. List of UE DataRadioBearerInfo by
   * DRBID.
   */
  std::map <uint8_t, Ptr<LteDataRadioBearerInfo> > m_drbMap;
  
  /**
   * Map the drb into a RLC (used for remote independent RLC in an MC setup)
   */
  std::map <uint8_t, Ptr<RlcBearerInfo> > m_rlcMap;
  /**
   * Map the drb into a RLCSetupRequest (used to handover remote independent RLC in an MC setup)
   */
  std::vector <EpcX2SapUser::RlcSetupRequest> m_rlcRequestVector;

  /**
   * The `Srb0` attribute. SignalingRadioBearerInfo for SRB0.
   */
  Ptr<LteSignalingRadioBearerInfo> m_srb0;
  /**
   * The `Srb1` attribute. SignalingRadioBearerInfo for SRB1.
   */
  Ptr<LteSignalingRadioBearerInfo> m_srb1;

  /**
   * The `C-RNTI` attribute. Cell Radio Network Temporary Identifier.
   */
  uint16_t m_rnti;
  /**
   * International Mobile Subscriber Identity assigned to this UE. A globally
   * unique UE identifier.
   */
  uint64_t m_imsi;
  ///
  uint8_t m_lastRrcTransactionIdentifier;
  ///
  LteRrcSap::PhysicalConfigDedicated m_physicalConfigDedicated;
  /// Pointer to the parent eNodeB RRC.
  Ptr<LteEnbRrc> m_rrc;
  /// The current UeManager state.
  State m_state;
  ///
  LtePdcpSapUser* m_drbPdcpSapUser;
  ///
  bool m_pendingRrcConnectionReconfiguration;

  /**
   * The `StateTransition` trace source. Fired upon every UE state transition
   * seen by the UeManager at the eNB RRC. Exporting IMSI, cell ID, RNTI, old
   * state, and new state.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t, State, State> m_stateTransitionTrace;

  /**
   * The `SwitchToMmWaveEnb` trace source. Fired upon receiving an ACK to a command to 
   * switch to MmWave RAT. Exporting IMSI, cellId, RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t> m_secondaryRlcCreatedTrace;

  /**
   * The `SetupDataRadioBearer' trace source. Fired upon receiving a request to
   * set up a DRB by the UeManager. Exporting IMSI, cellId, RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t> m_setupDataRadioBearerTrace;

  uint16_t m_sourceX2apId;
  uint16_t m_sourceCellId;
  uint16_t m_targetCellId;
  std::list<uint8_t> m_drbsToBeStarted;
  bool m_needPhyMacConfiguration;

  uint16_t m_mmWaveCellId;
  uint16_t m_mmWaveRnti;
  bool m_mmWaveCellAvailableForMcSetup;
  bool m_isMc;
  bool m_iab;
  bool m_needIabPhyMacConfiguration;
  //bool m_isInterRatHoCapable;

  /**
   * Time limit before a _connection request timeout_ occurs. Set after a new
   * UE context is added after a successful Random Access. Calling
   * LteEnbRrc::ConnectionRequestTimeout() when it expires. Cancelled when RRC
   * CONNECTION REQUEST is received.
   */
  EventId m_connectionRequestTimeout;
  /**
   * Time limit before a _connection setup timeout_ occurs. Set after an RRC
   * CONNECTION SETUP is sent. Calling LteEnbRrc::ConnectionSetupTimeout() when
   * it expires. Cancelled when RRC CONNECTION SETUP COMPLETE is received.
   */
  EventId m_connectionSetupTimeout;
  /**
   * The delay before a _connection rejected timeout_ occurs. Set after an RRC
   * CONNECTION REJECT is sent. Calling LteEnbRrc::ConnectionRejectedTimeout()
   * when it expires.
   */
  EventId m_connectionRejectedTimeout;
  /**
   * Time limit before a _handover joining timeout_ occurs. Set after a new UE
   * context is added after receiving a handover request. Calling
   * LteEnbRrc::HandoverJoiningTimeout() when it expires. Cancelled when
   * RRC CONNECTION RECONFIGURATION COMPLETE is received.
   */
  EventId m_handoverJoiningTimeout;
  /**
   * Time limit before a _handover leaving timeout_ occurs. Set after a
   * handover command is sent. Calling LteEnbRrc::HandoverLeavingTimeout()
   * when it expires. Cancelled when RRC CONNECTION RE-ESTABLISHMENT or X2
   * UE CONTEXT RELEASE is received.
   */
  EventId m_handoverLeavingTimeout;

  std::vector < Ptr<Packet> > m_x2forwardingBuffer;
  uint32_t m_x2forwardingBufferSize;
  uint32_t m_maxx2forwardingBufferSize;

  // this variable is set to true if on initial access, for mc devices, all the mmWave eNBs are in outage
  bool m_allMmWaveInOutageAtInitialAccess;

}; // end of `class UeManager`



/**
 * \ingroup lte
 * 
 * The LTE Radio Resource Control entity at the eNB
 */
class LteEnbRrc : public Object
{

  friend class EnbRrcMemberLteEnbCmacSapUser;
  friend class MemberLteHandoverManagementSapUser<LteEnbRrc>;
  friend class MemberLteAnrSapUser<LteEnbRrc>;
  friend class MemberLteFfrRrcSapUser<LteEnbRrc>;
  friend class MemberLteEnbRrcSapProvider<LteEnbRrc>;
  friend class MemberEpcEnbS1SapUser<LteEnbRrc>;
  friend class EpcX2SpecificEpcX2SapUser<LteEnbRrc>;
  friend class UeManager;
  friend class MemberLteEnbCphySapUser<LteEnbRrc>;

public:
  /**
   * create an RRC instance for use within an eNB
   *
   */
  LteEnbRrc ();

  /**
   * Destructor
   */
  virtual ~LteEnbRrc ();


  // inherited from Object
protected:
  virtual void DoDispose (void);
public:
  static TypeId GetTypeId (void);


  /**
   * Set the X2 SAP this RRC should interact with
   * \param s the X2 SAP Provider to be used by this RRC entity
   */
  void SetEpcX2SapProvider (EpcX2SapProvider* s);

  /** 
   * Get the X2 SAP offered by this RRC
   * \return s the X2 SAP User interface offered to the X2 entity by this RRC entity
   */
  EpcX2SapUser* GetEpcX2SapUser ();

  /**
   * Set the X2 PDCP Provider this RRC should pass to PDCP layers
   * \param s the X2 PDCP Provider to be stored in this RRC entity
   */
  void SetEpcX2PdcpProvider (EpcX2PdcpProvider* s);
  EpcX2PdcpProvider* GetEpcX2PdcpProvider () const;

  /**
   * Set the X2 RLC Provider this RRC should pass to RLC layers
   * \param s the X2 RLC Provider to be stored in this RRC entity
   */
  void SetEpcX2RlcProvider (EpcX2RlcProvider* s);
  EpcX2RlcProvider* GetEpcX2RlcProvider() const;

  /**
   * set the CMAC SAP this RRC should interact with
   *
   * \param s the CMAC SAP Provider to be used by this RRC
   */
  void SetLteEnbCmacSapProvider (LteEnbCmacSapProvider * s);

  /** 
   * Get the CMAC SAP offered by this RRC
   * \return s the CMAC SAP User interface offered to the MAC by this RRC
   */
  LteEnbCmacSapUser* GetLteEnbCmacSapUser ();


  /**
   * set the Handover Management SAP this RRC should interact with
   *
   * \param s the Handover Management SAP Provider to be used by this RRC
   */
  void SetLteHandoverManagementSapProvider (LteHandoverManagementSapProvider * s);

  /**
   * Get the Handover Management SAP offered by this RRC
   * \return s the Handover Management SAP User interface offered to the
   *           handover algorithm by this RRC
   */
  LteHandoverManagementSapUser* GetLteHandoverManagementSapUser ();


  /**
   * set the ANR SAP this RRC should interact with
   *
   * \param s the ANR SAP Provider to be used by this RRC
   */
  void SetLteAnrSapProvider (LteAnrSapProvider * s);

  /**
   * Get the ANR SAP offered by this RRC
   * \return s the ANR SAP User interface offered to the ANR instance by this
   *           RRC
   */
  LteAnrSapUser* GetLteAnrSapUser ();



  /**
   * set the FFR SAP this RRC should interact with
   *
   * \param s the FFR SAP Provider to be used by this RRC
   */
  void SetLteFfrRrcSapProvider (LteFfrRrcSapProvider * s);

  /**
   * Get the FFR SAP offered by this RRC
   * \return s the FFR SAP User interface offered to the ANR instance by this
   *           RRC
   */
  LteFfrRrcSapUser* GetLteFfrRrcSapUser ();

  /**
   * set the RRC SAP this RRC should interact with
   *
   * \param s the RRC SAP User to be used by this RRC
   */
  void SetLteEnbRrcSapUser (LteEnbRrcSapUser * s);

  /**
   *
   *
   * \return s the RRC SAP Provider interface offered to the MAC by this RRC
   */
  LteEnbRrcSapProvider* GetLteEnbRrcSapProvider ();

  /**
   * set the MAC SAP provider. The eNB RRC does not use this
   * directly, but it needs to provide it to newly created RLC instances.
   *
   * \param s the MAC SAP provider that will be used by all
   * newly created RLC instances
   */
  void SetLteMacSapProvider (LteMacSapProvider* s);


  /** 
   * Set the S1 SAP Provider
   * 
   * \param s the S1 SAP Provider
   */
  void SetS1SapProvider (EpcEnbS1SapProvider * s);

  /** 
   * 
   * \return the S1 SAP user
   */
  EpcEnbS1SapUser* GetS1SapUser ();


  /**
   * set the CPHY SAP this RRC should use to interact with the PHY
   *
   * \param s the CPHY SAP Provider
   */
  void SetLteEnbCphySapProvider (LteEnbCphySapProvider * s);

  /**
   *
   *
   * \return s the CPHY SAP User interface offered to the PHY by this RRC
   */
  LteEnbCphySapUser* GetLteEnbCphySapUser ();

  /** 
   * 
   * 
   * \param rnti the identifier of an UE
   * 
   * \return true if the corresponding UeManager instance exists
   */
  bool HasUeManager (uint16_t rnti) const;

  /**
   *
   *
   * \param rnti the identifier of an UE
   *
   * \return the corresponding UeManager instance
   */
  Ptr<UeManager> GetUeManager (uint16_t rnti);

  /**
   * \brief Add a new UE measurement reporting configuration
   * \param config the new reporting configuration
   * \return the measurement ID (measId) referring to the newly added
   *         reporting configuration
   *
   * Assuming intra-frequency environment, the new measurement reporting
   * configuration will be automatically associated to the only measurement
   * object (i.e., a new measurement identity will be automatically created).
   *
   * Can only be called before the start of simulation.
   */
  uint8_t AddUeMeasReportConfig (LteRrcSap::ReportConfigEutra config);

  /**
   * \brief Configure cell-specific parameters.
   * \param ulBandwidth the uplink bandwidth in number of RB
   * \param dlBandwidth the downlink bandwidth in number of RB
   * \param ulEarfcn the UL EARFCN
   * \param dlEarfcn the DL EARFCN
   * \param cellId the ID of the cell
   *
   * Configure cell-specific parameters and propagate them to lower layers.
   * The parameters include bandwidth, EARFCN (E-UTRA Absolute Radio Frequency
   * Channel Number), and cell ID.
   *
   * In addition to parameter configuration, this function also performs several
   * other tasks:
   *  - Initializing UE measurement (i.e. measurement object and quantity
   *    configuration), which is expected to be further configured through
   *    `LteEnbRrc::AddUeMeasReportConfig`;
   *  - Enabling MIB (Master Information Block) broadcast transmission
   *  - Enabling SIB (System Information Block) broadcast transmission
   *
   * Typically runs when the eNodeB NetDevice is installed, for instance by
   * `LteHelper::InstallEnbDevice` (i.e. before the simulation starts).
   *
   * \warning Raises an error when executed more than once.
   */
  void ConfigureCell (uint8_t ulBandwidth,
                      uint8_t dlBandwidth,
                      uint16_t ulEarfcn, 
                      uint16_t dlEarfcn,
                      uint16_t cellId);

  /** 
   * set the cell id of this eNB
   * 
   * \param m_cellId 
   */
  void SetCellId (uint16_t m_cellId);

  /** 
   * If this is a MmWave eNB RRC, set the cell id of the closest LTE cell
   * 
   * \param cellId 
   */
  void SetClosestLteCellId (uint16_t cellId);

  /** 
   * get the cell id of this eNB
   * 
   * \param m_cellId 
   */
  uint16_t GetCellId () const;

  /** 
   * Enqueue an IP data packet on the proper bearer for downlink
   * transmission. Normally expected to be called by the NetDevice
   * forwarding a packet coming from the EpcEnbApplication 
   * 
   * \param p the packet
   * 
   * \return true if successful, false if an error occurred
   */
  bool SendData (Ptr<Packet> p);

  /** 
   * set the callback used to forward data packets up the stack
   * 
   * \param cb 
   */
  void SetForwardUpCallback (Callback <void, Ptr<Packet> > cb);

  /** 
   * Method triggered when a UE is expected to request for connection but does
   * not do so in a reasonable time. The method will remove the UE context.
   * 
   * \param rnti the T-C-RNTI whose timeout expired
   */
  void ConnectionRequestTimeout (uint16_t rnti);

  /** 
   * Method triggered when a UE is expected to complete a connection setup
   * procedure but does not do so in a reasonable time. The method will remove
   * the UE context.
   *
   * \param rnti the T-C-RNTI whose timeout expired
   */
  void ConnectionSetupTimeout (uint16_t rnti);

  /**
   * Method triggered a while after sending RRC Connection Rejected. The method
   * will remove the UE context.
   * 
   * \param rnti the T-C-RNTI whose timeout expired
   */
  void ConnectionRejectedTimeout (uint16_t rnti);

  /** 
   * Method triggered when a UE is expected to join the cell for a handover 
   * but does not do so in a reasonable time. The method will remove the UE
   * context.
   * 
   * \param rnti the C-RNTI whose timeout expired
   */
  void HandoverJoiningTimeout (uint16_t rnti);

  /** 
   * Method triggered when a UE is expected to leave a cell for a handover
   * but no feedback is received in a reasonable time. The method will remove
   * the UE context.
   * 
   * \param rnti the C-RNTI whose timeout expired
   */
  void HandoverLeavingTimeout (uint16_t rnti);

  /** 
   * Send a HandoverRequest through the X2 SAP interface. This method will
   * trigger a handover which is started by the RRC by sending a handover
   * request to the target eNB over the X2 interface
   *
   * \param rnti the ID of the UE to be handed over
   * \param cellId the ID of the target eNB
   */
  void SendHandoverRequest (uint16_t rnti, uint16_t cellId);

  /**
   *  \brief This function acts as an interface to trigger Release indication messages towards eNB and EPC
   *  \param imsi the IMSI
   *  \param rnti the RNTI
   *  \param bearerId Bearer Identity which is to be de-activated
   */
  void DoSendReleaseDataRadioBearer (uint64_t imsi, uint16_t rnti, uint8_t bearerId);

  /**
   * Identifies how EPS Bearer parameters are mapped to different RLC types
   * 
   */
  enum LteEpsBearerToRlcMapping_t {RLC_SM_ALWAYS = 1,
                                   RLC_UM_ALWAYS = 2,
                                   RLC_AM_ALWAYS = 3,
                                   PER_BASED = 4,
																	 RLC_UM_LOWLAT_ALWAYS = 5};

  /**
   * TracedCallback signature for new Ue Context events.
   *
   * \param [in] cellId
   * \param [in] rnti
   */
  typedef void (* NewUeContextTracedCallback)
    (uint16_t cellId, uint16_t rnti);

  /**
   * TracedCallback signature for connection and handover end events.
   *
   * \param [in] imsi
   * \param [in] cellId
   * \param [in] rnti
   */
  typedef void (* ConnectionHandoverTracedCallback)
    (uint64_t imsi, uint16_t cellId, uint16_t rnti);
  
  /**
   * TracedCallback signature for handover start events.
   *
   * \param [in] imsi
   * \param [in] cellId
   * \param [in] rnti
   * \param [in] targetCid
   */
  typedef void (* HandoverStartTracedCallback)
    (uint64_t imsi, uint16_t cellId, uint16_t rnti, uint16_t targetCid);

  /**
   * TracedCallback signature for receive measurement report events.
   *
   * \param [in] imsi
   * \param [in] cellId
   * \param [in] rnti
   * \param [in] report
   * \todo The \c LteRrcSap::MeasurementReport argument should be
   * changed to a const reference since the argument is large.
   */
  typedef void (* ReceiveReportTracedCallback)
    (uint64_t imsi, uint16_t cellId, uint16_t rnti,
     LteRrcSap::MeasurementReport report);

  typedef void (* NotifyMmWaveSinrTracedCallback)
    (uint64_t imsi, uint16_t cellId, long double sinr);

  /**
   * Different secondary cell handover modes
   */
  enum HandoverMode
  {
    FIXED_TTT = 1,
    DYNAMIC_TTT = 2,
    THRESHOLD = 3
  };

  struct HandoverEventInfo
  {
    uint16_t sourceCellId;
    uint16_t targetCellId;
    EventId scheduledHandoverEvent;
  };
  
  /**
   * Map with info on handover events, per imsi
   */
  typedef std::map<uint64_t, HandoverEventInfo> HandoverEventMap;

  /**
   * This method maps Imsi to Rnti, so that the UeManager of a certain UE
   * can be retrieved also with the Imsi
   */
  void RegisterImsiToRnti(uint64_t imsi, uint16_t rnti);

  uint16_t GetRntiFromImsi(uint64_t imsi);

  uint64_t GetImsiFromRnti(uint16_t rnti);

  void SetInterRatHoMode ();
  
private:


  // RRC SAP methods

  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::CompleteSetupUe interface to UeManager::CompleteSetupUe
  void DoCompleteSetupUe (uint16_t rnti, LteEnbRrcSapProvider::CompleteSetupUeParameters params);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvRrcConnectionRequest interface to UeManager::RecvRrcConnectionRequest
  void DoRecvRrcConnectionRequest (uint16_t rnti, LteRrcSap::RrcConnectionRequest msg);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvRrcConnectionSetupCompleted interface to UeManager::RecvRrcConnectionSetupCompleted
  void DoRecvRrcConnectionSetupCompleted (uint16_t rnti, LteRrcSap::RrcConnectionSetupCompleted msg);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvRrcConnectionReconfigurationCompleted interface to UeManager::RecvRrcConnectionReconfigurationCompleted
  void DoRecvRrcConnectionReconfigurationCompleted (uint16_t rnti, LteRrcSap::RrcConnectionReconfigurationCompleted msg);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvRrcConnectionReestablishmentRequest interface to UeManager::RecvRrcConnectionReestablishmentRequest
  void DoRecvRrcConnectionReestablishmentRequest (uint16_t rnti, LteRrcSap::RrcConnectionReestablishmentRequest msg);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvRrcConnectionReestablishmentComplete interface to UeManager::RecvRrcConnectionReestablishmentComplete
  void DoRecvRrcConnectionReestablishmentComplete (uint16_t rnti, LteRrcSap::RrcConnectionReestablishmentComplete msg);
  /// Part of the RRC protocol. Forwarding LteEnbRrcSapProvider::RecvMeasurementReport interface to UeManager::RecvMeasurementReport
  void DoRecvMeasurementReport (uint16_t rnti, LteRrcSap::MeasurementReport msg);
  ///
  void DoRecvRrcSecondaryCellInitialAccessSuccessful (uint16_t rnti, uint16_t mmWaveRnti, uint16_t mmWaveCellId);
  
  // S1 SAP methods
  void DoDataRadioBearerSetupRequest (EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params);
  void DoPathSwitchRequestAcknowledge (EpcEnbS1SapUser::PathSwitchRequestAcknowledgeParameters params);
  void DoNotifyNumIabPerRnti (EpcEnbS1SapUser::NotifyNumIabPerRntiParameters params);

  // X2 SAP methods
  void DoRecvHandoverRequest (EpcX2SapUser::HandoverRequestParams params);
  void DoRecvHandoverRequestAck (EpcX2SapUser::HandoverRequestAckParams params);
  void DoRecvHandoverPreparationFailure (EpcX2SapUser::HandoverPreparationFailureParams params);
  void DoRecvSnStatusTransfer (EpcX2SapUser::SnStatusTransferParams params);
  void DoRecvUeContextRelease (EpcX2SapUser::UeContextReleaseParams params);
  void DoRecvLoadInformation (EpcX2SapUser::LoadInformationParams params);
  void DoRecvResourceStatusUpdate (EpcX2SapUser::ResourceStatusUpdateParams params);
  void DoRecvRlcSetupRequest (EpcX2SapUser::RlcSetupRequest params);
  void DoRecvRlcSetupCompleted (EpcX2SapUser::UeDataParams params);
  void DoRecvUeData (EpcX2SapUser::UeDataParams params);
  void DoRecvUeSinrUpdate(EpcX2SapUser::UeImsiSinrParams params);
  void DoRecvMcHandoverRequest(EpcX2SapUser::SecondaryHandoverParams params);
  void DoRecvLteMmWaveHandoverCompleted (EpcX2SapUser::SecondaryHandoverParams params);
  void DoRecvConnectionSwitchToMmWave (EpcX2SapUser::SwitchConnectionParams params);
  void DoRecvSecondaryCellHandoverCompleted (EpcX2SapUser::SecondaryHandoverCompletedParams params);

  // CMAC SAP methods

  uint16_t DoAllocateTemporaryCellRnti ();
  void DoNotifyLcConfigResult (uint16_t rnti, uint8_t lcid, bool success);
  void DoRrcConfigurationUpdateInd (LteEnbCmacSapUser::UeConfig params);

  // Handover Management SAP methods

  uint8_t DoAddUeMeasReportConfigForHandover (LteRrcSap::ReportConfigEutra reportConfig);
  void DoTriggerHandover (uint16_t rnti, uint16_t targetCellId);

  // ANR SAP methods

  uint8_t DoAddUeMeasReportConfigForAnr (LteRrcSap::ReportConfigEutra reportConfig);

  // FFR RRC SAP methods
  uint8_t DoAddUeMeasReportConfigForFfr (LteRrcSap::ReportConfigEutra reportConfig);
  void DoSetPdschConfigDedicated (uint16_t rnti, LteRrcSap::PdschConfigDedicated pa);
  void DoSendLoadInformation (EpcX2Sap::LoadInformationParams params);

  // CPHY SAP methods
  void DoUpdateUeSinrEstimate(LteEnbCphySapUser::UeAssociatedSinrInfo info);

  // Internal methods

  void EnableSwitching(uint64_t imsi);
  bool m_switchEnabled;

  /**
   * Allocate a new RNTI for a new UE. This is done in the following cases:
   *   * T-C-RNTI allocation upon contention-based MAC Random Access procedure
   *   * target cell RNTI allocation upon handover
   *
   * \param state the initial state of the UeManager
   *
   * \return the newly allocated RNTI
   */
  uint16_t AddUe (UeManager::State state);

  /**
   * remove a UE from the cell
   *
   * \param rnti the C-RNTI identiftying the user
   */
  void RemoveUe (uint16_t rnti);


  /** 
   * 
   * \param bearer the specification of an EPS bearer
   * 
   * \return the type of RLC that is to be created for the given EPS bearer
   */
  TypeId GetRlcType (EpsBearer bearer);



public:

  /** 
   * Add a neighbour with an X2 interface
   *
   * \param cellId neighbouring cell id
   */
  void AddX2Neighbour (uint16_t cellId);

  /** 
   * 
   * \param p the SRS periodicity in num TTIs
   */
  void SetSrsPeriodicity (uint32_t p);

  /** 
   *
   * \return the current SRS periodicity
   */
  uint32_t GetSrsPeriodicity () const;

  /**
   * \brief Associate this RRC entity with a particular CSG information.
   * \param csgId the intended Closed Subscriber Group identity
   * \param csgIndication if TRUE, only CSG members are allowed to access the
   *                      cell
   *
   * CSG identity is a number identifying a Closed Subscriber Group which the
   * cell belongs to. eNodeB is associated with a single CSG identity.
   *
   * The same CSG identity can also be associated to several UEs, which is
   * equivalent as enlisting these UEs as the members of this particular CSG.
   * When the CSG indication field is set to TRUE, only UEs which are members of
   * the CSG (i.e. same CSG ID) can gain access to the eNodeB, therefore
   * enforcing closed access mode. Otherwise, the eNodeB operates as a non-CSG
   * cell and implements open access mode.
   *
   * This restriction only applies to initial cell selection and EPC-enabled
   * simulation.
   */
  void SetCsgId (uint32_t csgId, bool csgIndication);

private:

  /** 
   * Allocate a new SRS configuration index for a new UE. 
   *
   * \note this method can have the side effect of updating the SRS
   * configuration index of all UEs
   * 
   * \return the newly allocated SRS configuration index
   */
  uint16_t GetNewSrsConfigurationIndex (void);

  /** 
   * remove a previously allocated SRS configuration index
   *
   * \note this method can have the side effect of updating the SRS
   * configuration index of all UEs
   * 
   * \param srcCi the indext to be removed
   */
  void RemoveSrsConfigurationIndex (uint16_t srcCi);

 

  /** 
   * 
   * \param bearer the characteristics of the bearer
   * 
   * \return the Logical Channel Group in a bearer with these
   * characteristics is put. Used for MAC Buffer Status Reporting purposes. 
   */
  uint8_t GetLogicalChannelGroup (EpsBearer bearer);
  uint8_t GetLogicalChannelGroup (bool isGbr);

  /** 
   * 
   * \param bearer the characteristics of the bearer
   * 
   * \return the priority level of a bearer with these
   * characteristics is put. Used for the part of UL MAC Scheduling
   * carried out by the UE
   */
  uint8_t GetLogicalChannelPriority (EpsBearer bearer);


  /** 
   * method used to periodically send System Information
   * 
   */
  void SendSystemInformation ();

  /** 
   * method used to periodically check if switching of MC devices or handover of MmWave dev is needed
   * 
   */
  void TriggerUeAssociationUpdate();

  /**
   * Trigger an handover according to certain conditions on the SINR
   * @params the iterator on m_imsiCellSinrMap
   * @params the sinrDifference between the current and the maxSinr cell
   * @params the CellId of the maximum SINR cell
   * @params the value of the SINR for this cell
   */
  void ThresholdBasedSecondaryCellHandover(std::map<uint64_t, CellSinrMap>::iterator imsiIter, double sinrDifference, uint16_t maxSinrCellId, double maxSinrDb);

    /**
   * Trigger an handover according to certain conditions on the SINR and the TTT
   * @params the iterator on m_imsiCellSinrMap
   * @params the sinrDifference between the current and the maxSinr cell
   * @params the CellId of the maximum SINR cell
   * @params the value of the SINR for this cell
   */
  void TttBasedHandover(std::map<uint64_t, CellSinrMap>::iterator imsiIter, double sinrDifference, uint16_t maxSinrCellId, double maxSinrDb);

  /**
   * Compute the TTT according to the sinrDifference and the dynamic handover algorithm
   */
  uint8_t ComputeTtt(double sinrDifference);

  /**
   * Method that can be scheduled to perform an handover
   * @params imsi 
   */
  void PerformHandover(uint64_t imsi);

  /** 
   * method used to periodically check if HO between MmWave and LTE is needed
   * 
   */
  void UpdateUeHandoverAssociation();

  /**
   * Trigger an handover according to certain conditions on the SINR (for single-connectivity devices)
   * @params the iterator on m_imsiCellSinrMap
   * @params the sinrDifference between the current and the maxSinr cell
   * @params the CellId of the maximum SINR cell
   * @params the value of the SINR for this cell
   */  
  void ThresholdBasedInterRatHandover(std::map<uint64_t, CellSinrMap>::iterator imsiIter, double sinrDifference, uint16_t maxSinrCellId, double maxSinrDb);
  
  Callback <void, Ptr<Packet> > m_forwardUpCallback;

  /// Interface to receive messages from neighbour eNodeB over the X2 interface.
  EpcX2SapUser* m_x2SapUser;
  /// Interface to send messages to neighbour eNodeB over the X2 interface.
  EpcX2SapProvider* m_x2SapProvider;
  /// Interface to be provided to PDCP
  EpcX2PdcpProvider* m_x2PdcpProvider;
  /// Interface to be provided to RLC
  EpcX2RlcProvider* m_x2RlcProvider;

  /// Receive API calls from the eNodeB MAC instance.
  LteEnbCmacSapUser* m_cmacSapUser;
  /// Interface to the eNodeB MAC instance.
  LteEnbCmacSapProvider* m_cmacSapProvider;

  /// Receive API calls from the handover algorithm instance.
  LteHandoverManagementSapUser* m_handoverManagementSapUser;
  /// Interface to the handover algorithm instance.
  LteHandoverManagementSapProvider* m_handoverManagementSapProvider;

  /// Receive API calls from the ANR instance.
  LteAnrSapUser* m_anrSapUser;
  /// Interface to the ANR instance.
  LteAnrSapProvider* m_anrSapProvider;

  /// Receive API calls from the FFR algorithm instance.
  LteFfrRrcSapUser* m_ffrRrcSapUser;
  /// Interface to the FFR algorithm instance.
  LteFfrRrcSapProvider* m_ffrRrcSapProvider;

  /// Interface to send messages to UE over the RRC protocol.
  LteEnbRrcSapUser* m_rrcSapUser;
  /// Interface to receive messages from UE over the RRC protocol.
  LteEnbRrcSapProvider* m_rrcSapProvider;

  /// Interface to the eNodeB MAC instance, to be used by RLC instances.
  LteMacSapProvider* m_macSapProvider;

  /// Interface to send messages to core network over the S1 protocol.
  EpcEnbS1SapProvider* m_s1SapProvider;
  /// Interface to receive messages from core network over the S1 protocol.
  EpcEnbS1SapUser* m_s1SapUser;

  /// Receive API calls from the eNodeB PHY instance.
  LteEnbCphySapUser* m_cphySapUser;
  /// Interface to the eNodeB PHY instance.
  LteEnbCphySapProvider* m_cphySapProvider;

  /// True if ConfigureCell() has been completed.
  bool m_configured;
  /// Cell identifier. Must be unique across the simulation.
  uint16_t m_cellId;
  /// Closest LTE Cell identifier (for MmWave cells only).
  uint16_t m_lteCellId;
  /// Downlink E-UTRA Absolute Radio Frequency Channel Number.
  uint16_t m_dlEarfcn;
  /// Uplink E-UTRA Absolute Radio Frequency Channel Number.
  uint16_t m_ulEarfcn;
  /// Downlink transmission bandwidth configuration in number of Resource Blocks.
  uint16_t m_dlBandwidth;
  /// Uplink transmission bandwidth configuration in number of Resource Blocks.
  uint16_t m_ulBandwidth;
  ///
  uint16_t m_lastAllocatedRnti;

  /// The System Information Block Type 1 that is currently broadcasted over BCH.
  LteRrcSap::SystemInformationBlockType1 m_sib1;

  /**
   * The `UeMap` attribute. List of UeManager by C-RNTI.
   */
  std::map<uint16_t, Ptr<UeManager> > m_ueMap;

  //std::map<uint16_t, EpcX2SapUser::HandoverRequestParams> m_requestMap;
  /**
   * List of measurement configuration which are active in every UE attached to
   * this eNodeB instance.
   */
  LteRrcSap::MeasConfig m_ueMeasConfig;

  /// List of measurement identities which are intended for handover purpose.
  std::set<uint8_t> m_handoverMeasIds;
  /// List of measurement identities which are intended for ANR purpose.
  std::set<uint8_t> m_anrMeasIds;
  /// List of measurement identities which are intended for FFR purpose.
  std::set<uint8_t> m_ffrMeasIds;

  struct X2uTeidInfo
  {
    uint16_t rnti;
    uint8_t drbid;
  };

  //       TEID      RNTI, DRBID
  std::map<uint32_t, X2uTeidInfo> m_x2uTeidInfoMap; // for the handovers
  std::map<uint32_t, X2uTeidInfo> m_x2uMcTeidInfoMap; // for the MC devices

  /**
   * The `DefaultTransmissionMode` attribute. The default UEs' transmission
   * mode (0: SISO).
   */
  uint8_t m_defaultTransmissionMode;
  /**
   * The `EpsBearerToRlcMapping` attribute. Specify which type of RLC will be
   * used for each type of EPS bearer.
   */
  enum LteEpsBearerToRlcMapping_t m_epsBearerToRlcMapping;
  /**
   * The `SystemInformationPeriodicity` attribute. The interval for sending
   * system information.
   */
  Time m_systemInformationPeriodicity;
  /**
   * The `SrsPeriodicity` attribute. The SRS periodicity in milliseconds.
   */
  uint16_t m_srsCurrentPeriodicityId;
  std::set<uint16_t> m_ueSrsConfigurationIndexSet;
  uint16_t m_lastAllocatedConfigurationIndex;
  bool m_reconfigureUes;

  /**
   * The `QRxLevMin` attribute. One of information transmitted within the SIB1
   * message, indicating the required minimum RSRP level that any UE must
   * receive from this cell before it is allowed to camp to this cell.
   */
  int8_t m_qRxLevMin;
  /**
   * The `AdmitHandoverRequest` attribute. Whether to admit an X2 handover
   * request from another eNB.
   */
  bool m_admitHandoverRequest;
  /**
   * The `AdmitRrcConnectionRequest` attribute. Whether to admit a connection
   * request from a UE.
   */
  bool m_admitRrcConnectionRequest;
  /**
   * The `RsrpFilterCoefficient` attribute. Determines the strength of
   * smoothing effect induced by layer 3 filtering of RSRP in all attached UE.
   * If equals to 0, no layer 3 filtering is applicable.
   */
  uint8_t m_rsrpFilterCoefficient;
  /**
   * The `RsrqFilterCoefficient` attribute. Determines the strength of
   * smoothing effect induced by layer 3 filtering of RSRQ in all attached UE.
   * If equals to 0, no layer 3 filtering is applicable.
   */
  uint8_t m_rsrqFilterCoefficient;
  /**
   * The `ConnectionRequestTimeoutDuration` attribute. After a RA attempt, if
   * no RRC CONNECTION REQUEST is received before this time, the UE context is
   * destroyed. Must account for reception of RAR and transmission of RRC
   * CONNECTION REQUEST over UL GRANT.
   */
  Time m_connectionRequestTimeoutDuration;
  /**
   * The `ConnectionSetupTimeoutDuration` attribute. After accepting connection
   * request, if no RRC CONNECTION SETUP COMPLETE is received before this time,
   * the UE context is destroyed. Must account for the UE's reception of RRC
   * CONNECTION SETUP and transmission of RRC CONNECTION SETUP COMPLETE.
   */
  Time m_connectionSetupTimeoutDuration;
  /**
   * The `ConnectionRejectedTimeoutDuration` attribute. Time to wait between
   * sending a RRC CONNECTION REJECT and destroying the UE context.
   */
  Time m_connectionRejectedTimeoutDuration;
  /**
   * The `HandoverJoiningTimeoutDuration` attribute. After accepting a handover
   * request, if no RRC CONNECTION RECONFIGURATION COMPLETE is received before
   * this time, the UE context is destroyed. Must account for reception of X2
   * HO REQ ACK by source eNB, transmission of the Handover Command,
   * non-contention-based random access and reception of the RRC CONNECTION
   * RECONFIGURATION COMPLETE message.
   */
  Time m_handoverJoiningTimeoutDuration;
  /**
   * The `HandoverLeavingTimeoutDuration` attribute. After issuing a Handover
   * Command, if neither RRC CONNECTION RE-ESTABLISHMENT nor X2 UE Context
   * Release has been previously received, the UE context is destroyed.
   */
  Time m_handoverLeavingTimeoutDuration;

  /**
   * The `NewUeContext` trace source. Fired upon creation of a new UE context.
   * Exporting cell ID and RNTI.
   */
  TracedCallback<uint16_t, uint16_t> m_newUeContextTrace;
  /**
   * The `ConnectionEstablished` trace source. Fired upon successful RRC
   * connection establishment. Exporting IMSI, cell ID, and RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t> m_connectionEstablishedTrace;
  /**
   * The `ConnectionReconfiguration` trace source. Fired upon RRC connection
   * reconfiguration. Exporting IMSI, cell ID, and RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t> m_connectionReconfigurationTrace;
  /**
   * The `HandoverStart` trace source. Fired upon start of a handover
   * procedure. Exporting IMSI, cell ID, RNTI, and target cell ID.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t, uint16_t> m_handoverStartTrace;
  /**
   * The `HandoverEndOk` trace source. Fired upon successful termination of a
   * handover procedure. Exporting IMSI, cell ID, and RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t> m_handoverEndOkTrace;
  /**
   * The `RecvMeasurementReport` trace source. Fired when measurement report is
   * received. Exporting IMSI, cell ID, and RNTI.
   */
  TracedCallback<uint64_t, uint16_t, uint16_t, LteRrcSap::MeasurementReport> m_recvMeasurementReportTrace;

  TracedCallback<uint64_t, uint16_t, long double> m_notifyMmWaveSinrTrace;

  bool m_ismmWave;
  bool m_interRatHoMode;
  bool m_firstReport;

  uint32_t m_firstSibTime;		// time in ms of initial SIB

  // for MmWave eNBs
  ImsiSinrMap m_ueImsiSinrMap;

  // for LTE eNBs
  std::map<uint16_t, ImsiSinrMap> m_cellSinrMap;
  uint16_t m_numNewSinrReports;
  std::map<uint64_t, uint16_t> m_bestMmWaveCellForImsiMap;
  std::map<uint64_t, uint16_t> m_lastMmWaveCell;
  std::map<uint64_t, bool> m_mmWaveCellSetupCompleted;
  std::map<uint64_t, bool> m_imsiUsingLte;
  std::map<uint64_t, CellSinrMap> m_imsiCellSinrMap;
  std::map<uint64_t, uint16_t> m_imsiRntiMap;
  std::map<uint16_t, uint64_t> m_rntiImsiMap;

  HandoverMode m_handoverMode;

  // TTT based handover management
  HandoverEventMap m_imsiHandoverEventsMap;

  long double m_sinrThresholdDifference;

  long double m_outageThreshold;

  uint8_t m_fixedTttValue;
  uint8_t m_minDynTttValue;
  uint8_t m_maxDynTttValue;
  double m_minDiffTttValue;
  double m_maxDiffTttValue;

  int m_crtPeriod;

  uint32_t m_x2_received_cnt;

}; // end of `class LteEnbRrc`


} // namespace ns3

#endif // LTE_ENB_RRC_H
