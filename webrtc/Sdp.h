//
// Created by xzl on 2021/3/27.
//

#ifndef ZLMEDIAKIT_SDP_H
#define ZLMEDIAKIT_SDP_H

#include <string>
#include <vector>
#include "Extension/Frame.h"
using namespace std;
using namespace mediakit;

//https://datatracker.ietf.org/doc/rfc4566/?include_text=1
//https://blog.csdn.net/aggresss/article/details/109850434
//https://aggresss.blog.csdn.net/article/details/106436703
//Session description
//         v=  (protocol version)
//         o=  (originator and session identifier)
//         s=  (session name)
//         i=* (session information)
//         u=* (URI of description)
//         e=* (email address)
//         p=* (phone number)
//         c=* (connection information -- not required if included in
//              all media)
//         b=* (zero or more bandwidth information lines)
//         One or more time descriptions ("t=" and "r=" lines; see below)
//         z=* (time zone adjustments)
//         k=* (encryption key)
//         a=* (zero or more session attribute lines)
//         Zero or more media descriptions
//
//      Time description
//         t=  (time the session is active)
//         r=* (zero or more repeat times)
//
//      Media description, if present
//         m=  (media name and transport address)
//         i=* (media title)
//         c=* (connection information -- optional if included at
//              session level)
//         b=* (zero or more bandwidth information lines)
//         k=* (encryption key)
//         a=* (zero or more media attribute lines)

enum class RtpDirection {
    invalid = -1,
    //只发送
    sendonly,
    //只接收
    recvonly,
    //同时发送接收
    sendrecv,
    //禁止发送数据
    inactive
};

enum class DtlsRole {
    invalid = -1,
    //客户端
    active,
    //服务端
    passive,
    //既可作做客户端也可以做服务端
    actpass,
};

enum class SdpType {
    invalid = -1,
    offer,
    answer
};

TrackType getTrackType(const string &str);
const char* getTrackString(TrackType type);
DtlsRole getDtlsRole(const string &str);
const char* getDtlsRoleString(DtlsRole role);
RtpDirection getRtpDirection(const string &str);
const char* getRtpDirectionString(RtpDirection val);

class SdpItem {
public:
    using Ptr = std::shared_ptr<SdpItem>;
    virtual ~SdpItem() = default;
    virtual void parse(const string &str) {
        value  = str;
    }
    virtual string toString() const {
        return value;
    }
    virtual const char* getKey() const = 0;

protected:
    mutable string value;
};

template <char KEY>
class SdpString : public SdpItem{
public:
    // *=*
    const char* getKey() const override { static string key(1, KEY); return key.data();}
};

class SdpCommon : public SdpItem {
public:
    string key;
    SdpCommon(string key) { this->key = std::move(key); }
    const char* getKey() const override { return key.data();}
};

class SdpTime : public SdpItem{
public:
    //5.9.  Timing ("t=")
    // t=<start-time> <stop-time>
    uint64_t start {0};
    uint64_t stop {0};
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "t";}
};

class SdpOrigin : public SdpItem{
public:
    // 5.2.  Origin ("o=")
    // o=jdoe 2890844526 2890842807 IN IP4 10.47.16.5
    // o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
    string username {"-"};
    string session_id;
    string session_version;
    string nettype {"IN"};
    string addrtype {"IP4"};
    string address {"0.0.0.0"};
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "o";}
};

class SdpConnection : public SdpItem {
public:
    // 5.7.  Connection Data ("c=")
    // c=IN IP4 224.2.17.12/127
    // c=<nettype> <addrtype> <connection-address>
    string nettype {"IN"};
    string addrtype {"IP4"};
    string address {"0.0.0.0"};
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "c";}
};

class SdpBandwidth : public SdpItem {
public:
    //5.8.  Bandwidth ("b=")
    //b=<bwtype>:<bandwidth>

    //AS、CT
    string bwtype {"AS"};
    uint32_t bandwidth {0};

    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "b";}
};

class SdpMedia : public SdpItem {
public:
    // 5.14.  Media Descriptions ("m=")
    // m=<media> <port> <proto> <fmt> ...
    TrackType type;
    uint16_t port;
    //RTP/AVP：应用场景为视频/音频的 RTP 协议。参考 RFC 3551
    //RTP/SAVP：应用场景为视频/音频的 SRTP 协议。参考 RFC 3711
    //RTP/AVPF: 应用场景为视频/音频的 RTP 协议，支持 RTCP-based Feedback。参考 RFC 4585
    //RTP/SAVPF: 应用场景为视频/音频的 SRTP 协议，支持 RTCP-based Feedback。参考 RFC 5124
    string proto;
    vector<uint32_t> fmts;

    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "m";}
};

class SdpAttr : public SdpItem{
public:
    using Ptr = std::shared_ptr<SdpAttr>;
    //5.13.  Attributes ("a=")
    //a=<attribute>
    //a=<attribute>:<value>
    SdpItem::Ptr detail;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "a";}
};

class SdpAttrGroup : public SdpItem{
public:
    //a=group:BUNDLE line with all the 'mid' identifiers part of the
    //  BUNDLE group is included at the session-level.
    //a=group:LS session level attribute MUST be included wth the 'mid'
    //  identifiers that are part of the same lip sync group.
    string type {"BUNDLE"};
    vector<string> mids;
    void parse(const string &str) override ;
    string toString() const override ;
    const char* getKey() const override { return "group";}
};

class SdpAttrMsidSemantic : public SdpItem {
public:
    //https://tools.ietf.org/html/draft-alvestrand-rtcweb-msid-02#section-3
    //3.  The Msid-Semantic Attribute
    //
    //   In order to fully reproduce the semantics of the SDP and SSRC
    //   grouping frameworks, a session-level attribute is defined for
    //   signalling the semantics associated with an msid grouping.
    //
    //   This OPTIONAL attribute gives the message ID and its group semantic.
    //     a=msid-semantic: examplefoo LS
    //
    //
    //   The ABNF of msid-semantic is:
    //
    //     msid-semantic-attr = "msid-semantic:" " " msid token
    //     token = <as defined in RFC 4566>
    //
    //   The semantic field may hold values from the IANA registries
    //   "Semantics for the "ssrc-group" SDP Attribute" and "Semantics for the
    //   "group" SDP Attribute".
    //a=msid-semantic: WMS 616cfbb1-33a3-4d8c-8275-a199d6005549
    string msid{"WMS"};
    string token;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "msid-semantic";}
};

class SdpAttrRtcp : public SdpItem {
public:
    // a=rtcp:9 IN IP4 0.0.0.0
    uint16_t port;
    string nettype {"IN"};
    string addrtype {"IP4"};
    string address {"0.0.0.0"};
    void parse(const string &str) override;;
    string toString() const override;
    const char* getKey() const override { return "rtcp";}
};

class SdpAttrIceUfrag : public SdpItem {
public:
    //a=ice-ufrag:sXJ3
    const char* getKey() const override { return "ice-ufrag";}
};

class SdpAttrIcePwd : public SdpItem {
public:
    //a=ice-pwd:yEclOTrLg1gEubBFefOqtmyV
    const char* getKey() const override { return "ice-pwd";}
};

class SdpAttrIceOption : public SdpItem {
public:
    //a=ice-options:trickle
    const char* getKey() const override { return "ice-options";}
};

class SdpAttrFingerprint : public SdpItem {
public:
    //a=fingerprint:sha-256 22:14:B5:AF:66:12:C7:C7:8D:EF:4B:DE:40:25:ED:5D:8F:17:54:DD:88:33:C0:13:2E:FD:1A:FA:7E:7A:1B:79
    string algorithm;
    string hash;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "fingerprint";}
};

class SdpAttrSetup : public SdpItem {
public:
    //a=setup:actpass
    DtlsRole role{DtlsRole::actpass};
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "setup";}
};

class SdpAttrMid : public SdpItem {
public:
    //a=mid:audio
    const char* getKey() const override { return "mid";}
};

class SdpAttrExtmap : public SdpItem {
public:
    //https://aggresss.blog.csdn.net/article/details/106436703
    //a=extmap:1[/sendonly] urn:ietf:params:rtp-hdrext:ssrc-audio-level
    int index;
    RtpDirection direction{RtpDirection::invalid};
    string ext;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "extmap";}
};

class SdpAttrRtpMap : public SdpItem {
public:
    //a=rtpmap:111 opus/48000/2
    uint8_t pt;
    string codec;
    int sample_rate;
    int channel {0};
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "rtpmap";}
};

class SdpAttrRtcpFb : public SdpItem {
public:
    //a=rtcp-fb:98 nack pli
    //a=rtcp-fb:120 nack 支持 nack 重传，nack (Negative-Acknowledgment) 。
    //a=rtcp-fb:120 nack pli 支持 nack 关键帧重传，PLI (Picture Loss Indication) 。
    //a=rtcp-fb:120 ccm fir 支持编码层关键帧请求，CCM (Codec Control Message)，FIR (Full Intra Request )，通常与 nack pli 有同样的效果，但是 nack pli 是用于重传时的关键帧请求。
    //a=rtcp-fb:120 goog-remb 支持 REMB (Receiver Estimated Maximum Bitrate) 。
    //a=rtcp-fb:120 transport-cc 支持 TCC (Transport Congest Control) 。
    uint8_t pt;
    vector<string> arr;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "rtcp-fb";}
};

class SdpAttrFmtp : public SdpItem {
public:
    //fmtp:96 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f
    uint8_t pt;
    vector<std::pair<string, string> > arr;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "fmtp";}
};

class SdpAttrSSRC : public SdpItem {
public:
    //a=ssrc:3245185839 cname:Cx4i/VTR51etgjT7
    //a=ssrc:3245185839 msid:cb373bff-0fea-4edb-bc39-e49bb8e8e3b9 0cf7e597-36a2-4480-9796-69bf0955eef5
    //a=ssrc:3245185839 mslabel:cb373bff-0fea-4edb-bc39-e49bb8e8e3b9
    //a=ssrc:3245185839 label:0cf7e597-36a2-4480-9796-69bf0955eef5
    //a=ssrc:<ssrc-id> <attribute>
    //a=ssrc:<ssrc-id> <attribute>:<value>
    //cname 是必须的，msid/mslabel/label 这三个属性都是 WebRTC 自创的，或者说 Google 自创的，可以参考 https://tools.ietf.org/html/draft-ietf-mmusic-msid-17，
    // 理解它们三者的关系需要先了解三个概念：RTP stream / MediaStreamTrack / MediaStream ：
    //一个 a=ssrc 代表一个 RTP stream ；
    //一个 MediaStreamTrack 通常包含一个或多个 RTP stream，例如一个视频 MediaStreamTrack 中通常包含两个 RTP stream，一个用于常规传输，一个用于 nack 重传；
    //一个 MediaStream 通常包含一个或多个 MediaStreamTrack ，例如 simulcast 场景下，一个 MediaStream 通常会包含三个不同编码质量的 MediaStreamTrack ；
    //这种标记方式并不被 Firefox 认可，在 Firefox 生成的 SDP 中一个 a=ssrc 通常只有一行，例如：
    //a=ssrc:3245185839 cname:Cx4i/VTR51etgjT7

    uint32_t ssrc;
    string attribute;
    string attribute_value;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "ssrc";}
};

class SdpAttrSSRCGroup : public SdpItem {
public:
    //a=ssrc-group 定义参考 RFC 5576(https://tools.ietf.org/html/rfc5576) ，用于描述多个 ssrc 之间的关联，常见的有两种：
    //a=ssrc-group:FID 2430709021 3715850271
    // FID (Flow Identification) 最初用在 FEC 的关联中，WebRTC 中通常用于关联一组常规 RTP stream 和 重传 RTP stream 。
    //a=ssrc-group:SIM 360918977 360918978 360918980
    // 在 Chrome 独有的 SDP munging 风格的 simulcast 中使用，将三组编码质量由低到高的 MediaStreamTrack 关联在一起。
    string type{"FID"};
    union {
        struct {
            uint32_t rtp_ssrc;
            uint32_t rtx_ssrc;
        } fid;
        struct {
            uint32_t rtp_ssrc_low;
            uint32_t rtp_ssrc_mid;
            uint32_t rtp_ssrc_high;
        } sim;
    } u;

    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "ssrc-group";}
};

class SdpAttrSctpMap : public SdpItem {
public:
    //https://tools.ietf.org/html/draft-ietf-mmusic-sctp-sdp-05
    //a=sctpmap:5000 webrtc-datachannel 1024
    //a=sctpmap: sctpmap-number media-subtypes [streams]
    uint16_t port;
    string subtypes;
    int streams;
    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "sctpmap";}
};

class SdpAttrCandidate : public SdpItem {
public:
    //https://tools.ietf.org/html/rfc5245
    //15.1.  "candidate" Attribute
    //a=candidate:4 1 udp 2 192.168.1.7 58107 typ host
    //a=candidate:<foundation> <component-id> <transport> <priority> <address> <port> typ <cand-type>
    uint32_t foundation;
    uint32_t component;
    string transport {"udp"};
    uint32_t priority;
    string address;
    uint16_t port;
    string type;
    vector<std::pair<string, string> > arr;

    void parse(const string &str) override;
    string toString() const override;
    const char* getKey() const override { return "candidate";}
};

class RtcSdpBase {
public:
    vector<SdpItem::Ptr> items;

public:
    virtual string toString() const;

    int getVersion() const;
    SdpOrigin getOrigin() const;
    string getSessionName() const;
    string getSessionInfo() const;
    SdpTime getSessionTime() const;
    SdpConnection getConnection() const;
    SdpBandwidth getBandwidth() const;

    string getUri() const;
    string getEmail() const;
    string getPhone() const;
    string getTimeZone() const;
    string getEncryptKey() const;
    string getRepeatTimes() const;
    RtpDirection getDirection() const;

private:
    SdpItem::Ptr getItem(char key, const char *attr_key = nullptr) const;

    template<typename cls>
    cls getItemClass(char key, const char *attr_key = nullptr) const{
        auto item = dynamic_pointer_cast<cls>(getItem(key, attr_key));
        if (!item) {
            return cls();
        }
        return *item;
    }

    string getStringItem(char key, const char *attr_key = nullptr) const{
        auto item = getItem(key, attr_key);
        if (!item) {
            return "";
        }
        return item->toString();
    }
};

class RtcSessionSdp : public RtcSdpBase{
public:
    vector<RtcSdpBase> medias;
    void parse(const string &str);
    string toString() const override;
};

//////////////////////////////////////////////////////////////////

//ssrc类型
enum class RtcSSRCType {
    rtp = 0,
    rtx,
    sim_low,
    sim_mid,
    ssrc_high
};

//ssrc相关信息
class RtcSSRC{
public:
    RtcSSRCType type;
    string cname;
    string msid;
    string mslabel;
    string label;
};

//rtc传输编码方案
class RtcPlan{
public:
    uint8_t pt;
    string codec;
    uint32_t sample_rate;
    //音频时有效
    uint32_t channel = 0;
    vector<string> rtcp_fb;
    vector<std::pair<string/*key*/, string/*value*/> > fmtp;
};

//rtc 媒体描述
class RtcMedia{
public:
    TrackType type;
    string mid;
    uint16_t port;
    string proto;

    //////// rtp ////////
    vector<RtcPlan> plan;
    SdpConnection rtp_addr;
    RtpDirection direction;
    RtcSSRC ssrc;

    //////// rtx - rtcp  ////////
    bool rtcp_mux;
    bool rtcp_rsize;
    uint32_t rtx_ssrc;
    SdpAttrRtcp rtcp_addr;

    //////// ice ////////
    bool ice_trickle;
    bool ice_lite;
    bool ice_renomination;
    string ice_ufrag;
    string ice_pwd;
    std::vector<SdpAttrCandidate> candidate;

    //////// dtls ////////
    DtlsRole role;
    SdpAttrFingerprint fingerprint;

    //////// extmap ////////
    vector<SdpAttrExtmap> extmap;
};

class RtcSession{
public:
    int version;
    SdpOrigin origin;
    string session_name;
    string session_info;
    SdpConnection connection;
    SdpBandwidth bandwidth;
    set<TrackType> group_bundle;
    vector<RtcMedia> media;
};


#endif //ZLMEDIAKIT_SDP_H