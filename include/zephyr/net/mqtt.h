/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt.h
 *
 * @brief MQTT Client Implementation
 *
 * @note The implementation assumes TCP module is enabled.
 *
 * @note By default the implementation uses MQTT version 3.1.1.
 *
 * @defgroup mqtt_socket MQTT Client library
 * @since 1.14
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_MQTT_H_
#define ZEPHYR_INCLUDE_NET_MQTT_H_

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/net/websocket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT Asynchronous Events notified to the application from the module
 *        through the callback registered by the application.
 */
enum mqtt_evt_type {
	/** Acknowledgment of connection request. Event result accompanying
	 *  the event indicates whether the connection failed or succeeded.
	 */
	MQTT_EVT_CONNACK,

	/** Disconnection Event. MQTT Client Reference is no longer valid once
	 *  this event is received for the client.
	 */
	MQTT_EVT_DISCONNECT,

	/** Publish event received when message is published on a topic client
	 *  is subscribed to.
	 *
	 * @note PUBLISH event structure only contains payload size, the payload
	 *       data parameter should be ignored. Payload content has to be
	 *       read manually with @ref mqtt_read_publish_payload function.
	 */
	MQTT_EVT_PUBLISH,

	/** Acknowledgment for published message with QoS 1. */
	MQTT_EVT_PUBACK,

	/** Reception confirmation for published message with QoS 2. */
	MQTT_EVT_PUBREC,

	/** Release of published message with QoS 2. */
	MQTT_EVT_PUBREL,

	/** Confirmation to a publish release message with QoS 2. */
	MQTT_EVT_PUBCOMP,

	/** Acknowledgment to a subscribe request. */
	MQTT_EVT_SUBACK,

	/** Acknowledgment to a unsubscribe request. */
	MQTT_EVT_UNSUBACK,

	/** Ping Response from server. */
	MQTT_EVT_PINGRESP,

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** Authentication packet received from server. MQTT 5.0 only. */
	MQTT_EVT_AUTH,
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief MQTT version protocol level. */
enum mqtt_version {
	MQTT_VERSION_3_1_0 = 3, /**< Protocol level for 3.1.0. */
	MQTT_VERSION_3_1_1 = 4, /**< Protocol level for 3.1.1. */
	MQTT_VERSION_5_0 = 5,   /**< Protocol level for 5.0. */
};

/** @brief MQTT Quality of Service types. */
enum mqtt_qos {
	/** Lowest Quality of Service, no acknowledgment needed for published
	 *  message.
	 */
	MQTT_QOS_0_AT_MOST_ONCE = 0x00,

	/** Medium Quality of Service, if acknowledgment expected for published
	 *  message, duplicate messages permitted.
	 */
	MQTT_QOS_1_AT_LEAST_ONCE = 0x01,

	/** Highest Quality of Service, acknowledgment expected and message
	 *  shall be published only once. Message not published to interested
	 *  parties unless client issues a PUBREL.
	 */
	MQTT_QOS_2_EXACTLY_ONCE  = 0x02
};

/** @brief MQTT 3.1 CONNACK return codes. */
enum mqtt_conn_return_code {
	/** Connection accepted. */
	MQTT_CONNECTION_ACCEPTED                = 0x00,

	/** The Server does not support the level of the MQTT protocol
	 * requested by the Client.
	 */
	MQTT_UNACCEPTABLE_PROTOCOL_VERSION      = 0x01,

	/** The Client identifier is correct UTF-8 but not allowed by the
	 *  Server.
	 */
	MQTT_IDENTIFIER_REJECTED                = 0x02,

	/** The Network Connection has been made but the MQTT service is
	 *  unavailable.
	 */
	MQTT_SERVER_UNAVAILABLE                 = 0x03,

	/** The data in the user name or password is malformed. */
	MQTT_BAD_USER_NAME_OR_PASSWORD          = 0x04,

	/** The Client is not authorized to connect. */
	MQTT_NOT_AUTHORIZED                     = 0x05
};

/** @brief MQTT 5.0 CONNACK reason codes (MQTT 5.0, chapter 3.2.2.2). */
enum mqtt_connack_reason_code {
	MQTT_CONNACK_SUCCESS = 0,
	MQTT_CONNACK_UNSPECIFIED_ERROR = 128,
	MQTT_CONNACK_MALFORMED_PACKET = 129,
	MQTT_CONNACK_PROTOCOL_ERROR = 130,
	MQTT_CONNACK_IMPL_SPECIFIC_ERROR = 131,
	MQTT_CONNACK_UNSUPPORTED_PROTO_ERROR = 132,
	MQTT_CONNACK_CLIENT_ID_NOT_VALID = 133,
	MQTT_CONNACK_BAD_USERNAME_OR_PASS = 134,
	MQTT_CONNACK_NOT_AUTHORIZED = 135,
	MQTT_CONNACK_SERVER_UNAVAILABLE = 136,
	MQTT_CONNACK_SERVER_BUSY = 137,
	MQTT_CONNACK_BANNED = 138,
	MQTT_CONNACK_BAD_AUTH_METHOD = 140,
	MQTT_CONNACK_TOPIC_NAME_INVALID = 144,
	MQTT_CONNACK_PACKET_TOO_LARGE = 149,
	MQTT_CONNACK_QUOTA_EXCEEDED = 151,
	MQTT_CONNACK_PAYLOAD_FORMAT_INVALID = 153,
	MQTT_CONNACK_RETAIN_NOT_SUPPORTED = 154,
	MQTT_CONNACK_QOS_NOT_SUPPORTED = 155,
	MQTT_CONNACK_USE_ANOTHER_SERVER = 156,
	MQTT_CONNACK_SERVER_MOVED = 157,
	MQTT_CONNACK_CONNECTION_RATE_EXCEEDED = 159,
};

/** @brief MQTT SUBACK return codes. */
enum mqtt_suback_return_code {
	/** Subscription with QoS 0 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_0 = 0x00,

	/** Subscription with QoS 1 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_1 = 0x01,

	/** Subscription with QoS 2 succeeded. */
	MQTT_SUBACK_SUCCESS_QoS_2 = 0x02,

	/** Subscription for a topic failed. */
	MQTT_SUBACK_FAILURE = 0x80
};

/** @brief MQTT Disconnect reason codes (MQTT 5.0, chapter 3.14.2.1). */
enum mqtt_disconnect_reason_code {
	MQTT_DISCONNECT_NORMAL = 0,
	MQTT_DISCONNECT_WITH_WILL_MSG = 4,
	MQTT_DISCONNECT_UNSPECIFIED_ERROR = 128,
	MQTT_DISCONNECT_MALFORMED_PACKET = 129,
	MQTT_DISCONNECT_PROTOCOL_ERROR = 130,
	MQTT_DISCONNECT_IMPL_SPECIFIC_ERROR = 131,
	MQTT_DISCONNECT_NOT_AUTHORIZED = 135,
	MQTT_DISCONNECT_SERVER_BUSY = 137,
	MQTT_DISCONNECT_SERVER_SHUTTING_DOWN = 139,
	MQTT_DISCONNECT_KEEP_ALIVE_TIMEOUT = 141,
	MQTT_DISCONNECT_SESSION_TAKE_OVER = 142,
	MQTT_DISCONNECT_TOPIC_FILTER_INVALID = 143,
	MQTT_DISCONNECT_TOPIC_NAME_INVALID = 144,
	MQTT_DISCONNECT_RECV_MAX_EXCEEDED = 147,
	MQTT_DISCONNECT_TOPIC_ALIAS_INVALID = 148,
	MQTT_DISCONNECT_PACKET_TOO_LARGE = 149,
	MQTT_DISCONNECT_MESSAGE_RATE_TOO_HIGH = 150,
	MQTT_DISCONNECT_QUOTA_EXCEEDED = 151,
	MQTT_DISCONNECT_ADMIN_ACTION = 152,
	MQTT_DISCONNECT_PAYLOAD_FORMAT_INVALID = 153,
	MQTT_DISCONNECT_RETAIN_NOT_SUPPORTED = 154,
	MQTT_DISCONNECT_QOS_NOT_SUPPORTED = 155,
	MQTT_DISCONNECT_USE_ANOTHER_SERVER = 156,
	MQTT_DISCONNECT_SERVER_MOVED = 157,
	MQTT_DISCONNECT_SHARED_SUB_NOT_SUPPORTED = 158,
	MQTT_DISCONNECT_CONNECTION_RATE_EXCEEDED = 159,
	MQTT_DISCONNECT_MAX_CONNECT_TIME = 160,
	MQTT_DISCONNECT_SUB_ID_NOT_SUPPORTED = 161,
	MQTT_DISCONNECT_WILDCARD_SUB_NOT_SUPPORTED = 162,
};

/** @brief MQTT Authenticate reason codes (MQTT 5.0, chapter 3.15.2.1). */
enum mqtt_auth_reason_code {
	MQTT_AUTH_SUCCESS = 0,
	MQTT_AUTH_CONTINUE_AUTHENTICATION = 24,
	MQTT_AUTH_RE_AUTHENTICATE = 25,
};

/** @brief Abstracts UTF-8 encoded strings. */
struct mqtt_utf8 {
	const uint8_t *utf8;       /**< Pointer to UTF-8 string. */
	uint32_t size;             /**< Size of UTF string, in bytes. */
};

/**
 * @brief Initialize UTF-8 encoded string from C literal string.
 *
 * Use it as follows:
 *
 * struct mqtt_utf8 password = MQTT_UTF8_LITERAL("my_pass");
 *
 * @param[in] literal Literal string from which to generate mqtt_utf8 object.
 */
#define MQTT_UTF8_LITERAL(literal)				\
	((struct mqtt_utf8) {literal, sizeof(literal) - 1})

/** @brief Abstracts binary strings. */
struct mqtt_binstr {
	uint8_t *data;             /**< Pointer to binary stream. */
	uint32_t len;              /**< Length of binary stream. */
};

/** @brief Abstracts aliased topic. */
struct mqtt_topic_alias {
#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** UTF-8 encoded topic name. */
	uint8_t topic_buf[CONFIG_MQTT_TOPIC_ALIAS_STRING_MAX];

	/** Topic name size. */
	uint16_t topic_size;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Abstracts MQTT UTF-8 encoded topic that can be subscribed
 *         to or published.
 */
struct mqtt_topic {
	/** Topic on to be published or subscribed to. */
	struct mqtt_utf8 topic;

	/** Quality of service requested for the subscription.
	 *  @ref mqtt_qos for details.
	 */
	uint8_t qos;
};

/** @brief Abstracts MQTT UTF-8 encoded string pair.
 */
struct mqtt_utf8_pair {
	struct mqtt_utf8 name;
	struct mqtt_utf8 value;
};

/** @brief Parameters for a publish message. */
struct mqtt_publish_message {
	struct mqtt_topic topic;     /**< Topic on which data was published. */
	struct mqtt_binstr payload; /**< Payload on the topic published. */
};

/** @brief Parameters for a connection acknowledgment (CONNACK). */
struct mqtt_connack_param {
	/** The Session Present flag enables a Client to establish whether
	 *  the Client and Server have a consistent view about whether there
	 *  is already stored Session state.
	 */
	uint8_t session_present_flag;

	/** The appropriate non-zero Connect return code indicates if the Server
	 *  is unable to process a connection request for some reason.
	 *  MQTT 3.1 - Return codes specified in @ref mqtt_conn_return_code
	 *  MQTT 5.0 - Reason codes specified in @ref mqtt_connack_reason_code
	 */
	uint8_t return_code;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	struct {
		/** MQTT 5.0, chapter 3.2.2.3.10 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.2.2.3.7 Assigned Client Identifier. */
		struct mqtt_utf8 assigned_client_id;

		/** MQTT 5.0, chapter 3.2.2.3.9 Reason String. */
		struct mqtt_utf8 reason_string;

		/** MQTT 5.0, chapter 3.2.2.3.15 Response Information. */
		struct mqtt_utf8 response_information;

		/** MQTT 5.0, chapter 3.2.2.3.16 Server Reference. */
		struct mqtt_utf8 server_reference;

		/** MQTT 5.0, chapter 3.2.2.3.17 Authentication Method. */
		struct mqtt_utf8 auth_method;

		/** MQTT 5.0, chapter 3.2.2.3.18 Authentication Data. */
		struct mqtt_binstr auth_data;

		/** MQTT 5.0, chapter 3.2.2.3.2 Session Expiry Interval. */
		uint32_t session_expiry_interval;

		/** MQTT 5.0, chapter 3.2.2.3.6 Maximum Packet Size. */
		uint32_t maximum_packet_size;

		/** MQTT 5.0, chapter 3.3.2.3.3 Receive Maximum. */
		uint16_t receive_maximum;

		/** MQTT 5.0, chapter 3.2.2.3.8 Topic Alias Maximum. */
		uint16_t topic_alias_maximum;

		/** MQTT 5.0, chapter 3.2.2.3.14 Server Keep Alive. */
		uint16_t server_keep_alive;

		/** MQTT 5.0, chapter 3.2.2.3.4 Maximum QoS. */
		uint8_t maximum_qos;

		/** MQTT 5.0, chapter 3.2.2.3.5 Retain Available. */
		uint8_t retain_available;

		/** MQTT 5.0, chapter 3.2.2.3.11 Wildcard Subscription Available. */
		uint8_t wildcard_sub_available;

		/** MQTT 5.0, chapter 3.2.2.3.12 Subscription Identifiers Available. */
		uint8_t subscription_ids_available;

		/** MQTT 5.0, chapter 3.2.2.3.13 Shared Subscription Available. */
		uint8_t shared_sub_available;

		/** Flags indicating whether given property was present in received packet. */
		struct {
			/** Session Expiry Interval property was present. */
			bool has_session_expiry_interval;
			/** Receive Maximum property was present. */
			bool has_receive_maximum;
			/** Maximum QoS property was present. */
			bool has_maximum_qos;
			/** Retain Available property was present. */
			bool has_retain_available;
			/** Maximum Packet Size property was present. */
			bool has_maximum_packet_size;
			/** Assigned Client Identifier property was present. */
			bool has_assigned_client_id;
			/** Topic Alias Maximum property was present. */
			bool has_topic_alias_maximum;
			/** Reason String property was present. */
			bool has_reason_string;
			/** User Property property was present. */
			bool has_user_prop;
			/** Wildcard Subscription Available property was present. */
			bool has_wildcard_sub_available;
			/** Subscription Identifiers Available property was present. */
			bool has_subscription_ids_available;
			/** Shared Subscription Available property was present. */
			bool has_shared_sub_available;
			/** Server Keep Alive property was present. */
			bool has_server_keep_alive;
			/** Response Information property was present. */
			bool has_response_information;
			/** Server Reference property was present. */
			bool has_server_reference;
			/** Authentication Method property was present. */
			bool has_auth_method;
			/** Authentication Data property was present. */
			bool has_auth_data;
		} rx;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Common MQTT 5.0 properties shared across all ack-type messages. */
struct mqtt_common_ack_properties {
#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0, chapter 3.4.2.2.3 User Property. */
	struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

	/** MQTT 5.0, chapter 3.4.2.2.2 Reason String. */
	struct mqtt_utf8 reason_string;

	/** Flags indicating whether given property was present in received packet. */
	struct {
		/** Reason String property was present. */
		bool has_reason_string;
		/** User Property property was present. */
		bool has_user_prop;
	} rx;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT publish acknowledgment (PUBACK). */
struct mqtt_puback_param {
	/** Message id of the PUBLISH message being acknowledged */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 reason code. */
	uint8_t reason_code;

	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT publish receive (PUBREC). */
struct mqtt_pubrec_param {
	/** Message id of the PUBLISH message being acknowledged */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 reason code. */
	uint8_t reason_code;

	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT publish release (PUBREL). */
struct mqtt_pubrel_param {
	/** Message id of the PUBREC message being acknowledged */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 reason code. */
	uint8_t reason_code;

	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT publish complete (PUBCOMP). */
struct mqtt_pubcomp_param {
	/** Message id of the PUBREL message being acknowledged */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 reason code. */
	uint8_t reason_code;

	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT subscription acknowledgment (SUBACK). */
struct mqtt_suback_param {
	/** Message id of the SUBSCRIBE message being acknowledged */
	uint16_t message_id;

	/** MQTT 3.1 - Return codes indicating maximum QoS level granted for
	 *  each topic in the subscription list.
	 *  MQTT 5.0 - Reason codes corresponding to each topic in the
	 *  subscription list.
	 */
	struct mqtt_binstr return_codes;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for MQTT unsubscribe acknowledgment (UNSUBACK). */
struct mqtt_unsuback_param {
	/** Message id of the UNSUBSCRIBE message being acknowledged */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** Reason codes corresponding to each topic in the unsubscription list. */
	struct mqtt_binstr reason_codes;

	/** MQTT 5.0 properties. */
	struct mqtt_common_ack_properties prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for a publish message (PUBLISH). */
struct mqtt_publish_param {
	/** Messages including topic, QoS and its payload (if any)
	 *  to be published.
	 */
	struct mqtt_publish_message message;

	/** Message id used for the publish message. Redundant for QoS 0. */
	uint16_t message_id;

	/** Duplicate flag. If 1, it indicates the message is being
	 *  retransmitted. Has no meaning with QoS 0.
	 */
	uint8_t dup_flag : 1;

	/** Retain flag. If 1, the message shall be stored persistently
	 *  by the broker.
	 */
	uint8_t retain_flag : 1;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 properties. */
	struct {
		/** MQTT 5.0, chapter 3.3.2.3.7 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.3.2.3.5 Response Topic. */
		struct mqtt_utf8 response_topic;

		/** MQTT 5.0, chapter 3.3.2.3.6 Correlation Data. */
		struct mqtt_binstr correlation_data;

		/** MQTT 5.0, chapter 3.3.2.3.9 Content Type. */
		struct mqtt_utf8 content_type;

		/** MQTT 5.0, chapter 3.3.2.3.8 Subscription Identifier. */
		uint32_t subscription_identifier[CONFIG_MQTT_SUBSCRIPTION_ID_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.3.2.3.3 Message Expiry Interval. */
		uint32_t message_expiry_interval;

		/** MQTT 5.0, chapter 3.3.2.3.4 Topic Alias. */
		uint16_t topic_alias;

		/** MQTT 5.0, chapter 3.3.2.3.2 Payload Format Indicator. */
		uint8_t payload_format_indicator;

		/** Flags indicating whether given property was present in received packet. */
		struct {
			/** Payload Format Indicator property was present. */
			bool has_payload_format_indicator;
			/** Message Expiry Interval property was present. */
			bool has_message_expiry_interval;
			/** Topic Alias property was present. */
			bool has_topic_alias;
			/** Response Topic property was present. */
			bool has_response_topic;
			/** Correlation Data property was present. */
			bool has_correlation_data;
			/** User Property property was present. */
			bool has_user_prop;
			/** Subscription Identifier property was present. */
			bool has_subscription_identifier;
			/** Content Type property was present. */
			bool has_content_type;
		} rx;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for subscribe/unsubscribe message. */
struct mqtt_subscription_list {
	/** Array containing topics along with QoS for each. */
	struct mqtt_topic *list;

	/** Number of topics in the subscription list */
	uint16_t list_count;

	/** Message id used to identify subscription request. */
	uint16_t message_id;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 properties. */
	struct {
		/** MQTT 5.0, chapter 3.8.2.1.3 / 3.10.2.1.2 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.8.2.1.2 Subscription Identifier.
		 *  Ignored for UNSUBSCRIBE requests.
		 */
		uint32_t subscription_identifier;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for disconnect message. */
struct mqtt_disconnect_param {
#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/* MQTT 5.0 Disconnect reason code. */
	enum mqtt_disconnect_reason_code reason_code;

	/** MQTT 5.0 properties. */
	struct {
		/** MQTT 5.0, chapter 3.14.2.2.4 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.14.2.2.3 Reason String. */
		struct mqtt_utf8 reason_string;

		/** MQTT 5.0, chapter 3.14.2.2.5 Server Reference. */
		struct mqtt_utf8 server_reference;

		/** MQTT 5.0, chapter 3.14.2.2.2 Session Expiry Interval. */
		uint32_t session_expiry_interval;

		/** Flags indicating whether given property was present in received packet. */
		struct {
			/** Session Expiry Interval property was present. */
			bool has_session_expiry_interval;
			/** Reason String property was present. */
			bool has_reason_string;
			/** User Property property was present. */
			bool has_user_prop;
			/** Server Reference property was present. */
			bool has_server_reference;
		} rx;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Parameters for auth message. */
struct mqtt_auth_param {
#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/* MQTT 5.0, chapter 3.15.2.1 Authenticate Reason Code */
	enum mqtt_auth_reason_code reason_code;

	struct {
		/** MQTT 5.0, chapter 3.15.2.2.5 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.15.2.2.2 Authentication Method. */
		struct mqtt_utf8 auth_method;

		/** MQTT 5.0, chapter 3.15.2.2.3 Authentication Data. */
		struct mqtt_binstr auth_data;

		/** MQTT 5.0, chapter 3.15.2.2.4 Reason String. */
		struct mqtt_utf8 reason_string;

		/** Flags indicating whether given property was present in received packet. */
		struct {
			/** Authentication Method property was present. */
			bool has_auth_method;
			/** Authentication Data property was present. */
			bool has_auth_data;
			/** Reason String property was present. */
			bool has_reason_string;
			/** User Property property was present. */
			bool has_user_prop;
		} rx;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/**
 * @brief Defines event parameters notified along with asynchronous events
 *        to the application.
 */
union mqtt_evt_param {
	/** Parameters accompanying MQTT_EVT_CONNACK event. */
	struct mqtt_connack_param connack;

	/** Parameters accompanying MQTT_EVT_PUBLISH event.
	 *
	 * @note PUBLISH event structure only contains payload size, the payload
	 *       data parameter should be ignored. Payload content has to be
	 *       read manually with @ref mqtt_read_publish_payload function.
	 */
	struct mqtt_publish_param publish;

	/** Parameters accompanying MQTT_EVT_PUBACK event. */
	struct mqtt_puback_param puback;

	/** Parameters accompanying MQTT_EVT_PUBREC event. */
	struct mqtt_pubrec_param pubrec;

	/** Parameters accompanying MQTT_EVT_PUBREL event. */
	struct mqtt_pubrel_param pubrel;

	/** Parameters accompanying MQTT_EVT_PUBCOMP event. */
	struct mqtt_pubcomp_param pubcomp;

	/** Parameters accompanying MQTT_EVT_SUBACK event. */
	struct mqtt_suback_param suback;

	/** Parameters accompanying MQTT_EVT_UNSUBACK event. */
	struct mqtt_unsuback_param unsuback;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** Parameters accompanying MQTT_EVT_DISCONNECT event. */
	struct mqtt_disconnect_param disconnect;

	/** Parameters accompanying MQTT_EVT_AUTH event. */
	struct mqtt_auth_param auth;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/** @brief Defines MQTT asynchronous event notified to the application. */
struct mqtt_evt {
	/** Identifies the event. */
	enum mqtt_evt_type type;

	/** Contains parameters (if any) accompanying the event. */
	union mqtt_evt_param param;

	/** Event result. 0 or a negative error code (errno.h) indicating
	 *  reason of failure.
	 */
	int result;
};

struct mqtt_client;

/**
 * @brief Asynchronous event notification callback registered by the
 *        application.
 *
 * @param[in] client Identifies the client for which the event is notified.
 * @param[in] evt Event description along with result and associated
 *                parameters (if any).
 */
typedef void (*mqtt_evt_cb_t)(struct mqtt_client *client,
			      const struct mqtt_evt *evt);

/** @brief TLS configuration for secure MQTT transports. */
struct mqtt_sec_config {
	/** Indicates the preference for peer verification. */
	int peer_verify;

	/** Indicates the number of entries in the cipher list. */
	uint32_t cipher_count;

	/** Indicates the list of ciphers to be used for the session.
	 *  May be NULL to use the default ciphers.
	 */
	const int *cipher_list;

	/** Indicates the number of entries in the sec tag list. */
	uint32_t sec_tag_count;

	/** Indicates the list of security tags to be used for the session. */
	const sec_tag_t *sec_tag_list;

#if defined(CONFIG_MQTT_LIB_TLS_USE_ALPN)
	/**
	 * Pointer to array of string indicating the ALPN protocol name.
	 * May be NULL to skip ALPN protocol negotiation.
	 */
	const char **alpn_protocol_name_list;

	/**
	 * Indicate number of ALPN protocol name in alpn protocol name list.
	 */
	uint32_t alpn_protocol_name_count;
#endif

	/** Indicates the preference for enabling TLS session caching. */
	int session_cache;

	/** Peer hostname for ceritificate verification.
	 *  May be NULL to skip hostname verification.
	 */
	const char *hostname;

	/** Indicates the preference for copying certificates to the heap. */
	int cert_nocopy;

	/** Set socket to native TLS */
	bool set_native_tls;
};

/** @brief MQTT transport type. */
enum mqtt_transport_type {
	/** Use non secure TCP transport for MQTT connection. */
	MQTT_TRANSPORT_NON_SECURE,

#if defined(CONFIG_MQTT_LIB_TLS)
	/** Use secure TCP transport (TLS) for MQTT connection. */
	MQTT_TRANSPORT_SECURE,
#endif /* CONFIG_MQTT_LIB_TLS */

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	/** Use non secure Websocket transport for MQTT connection. */
	MQTT_TRANSPORT_NON_SECURE_WEBSOCKET,
#if defined(CONFIG_MQTT_LIB_TLS)
	/** Use secure Websocket transport (TLS) for MQTT connection. */
	MQTT_TRANSPORT_SECURE_WEBSOCKET,
#endif
#endif /* CONFIG_MQTT_LIB_WEBSOCKET */
#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
	/** Use custom transport for MQTT connection. */
	MQTT_TRANSPORT_CUSTOM,
#endif /* CONFIG_MQTT_LIB_CUSTOM_TRANSPORT */

	/** Shall not be used as a transport type.
	 *  Indicator of maximum transport types possible.
	 */
	MQTT_TRANSPORT_NUM
};

/** @brief MQTT transport specific data. */
struct mqtt_transport {
	/** Transport type selection for client instance.
	 *  @ref mqtt_transport_type for possible values. MQTT_TRANSPORT_MAX
	 *  is not a valid type.
	 */
	enum mqtt_transport_type type;

	/** Name of the interface that the MQTT client instance should be bound to.
	 *  Leave as NULL if not specified.
	 */
	const char *if_name;

	/** Use either unsecured TCP or secured TLS transport */
	union {
		/** TCP socket transport for MQTT */
		struct {
			/** Socket descriptor. */
			int sock;
		} tcp;

#if defined(CONFIG_MQTT_LIB_TLS)
		/** TLS socket transport for MQTT */
		struct {
			/** Socket descriptor. */
			int sock;

			/** TLS configuration. See @ref mqtt_sec_config for
			 *  details.
			 */
			struct mqtt_sec_config config;
		} tls;
#endif /* CONFIG_MQTT_LIB_TLS */
	};

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	/** Websocket transport for MQTT */
	struct {
		/** Websocket configuration. */
		struct websocket_request config;

		/** Socket descriptor */
		int sock;

		/** Websocket timeout, in milliseconds. */
		int32_t timeout;
	} websocket;
#endif

#if defined(CONFIG_MQTT_LIB_CUSTOM_TRANSPORT)
	/** User defined data for custom transport for MQTT. */
	void *custom_transport_data;
#endif /* CONFIG_MQTT_LIB_CUSTOM_TRANSPORT */

#if defined(CONFIG_SOCKS)
	struct {
		struct sockaddr addr;
		socklen_t addrlen;
	} proxy;
#endif
};

/** @brief MQTT internal state. */
struct mqtt_internal {
	/** Internal. Mutex to protect access to the client instance. */
	struct sys_mutex mutex;

	/** Internal. Wall clock value (in milliseconds) of the last activity
	 *  that occurred. Needed for periodic PING.
	 */
	uint32_t last_activity;

	/** Internal. Client's state in the connection. */
	uint32_t state;

	/** Internal.  Packet length read so far. */
	uint32_t rx_buf_datalen;

	/** Internal. Remaining payload length to read. */
	uint32_t remaining_payload;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** Internal. MQTT 5.0 topic alias mapping. */
	struct mqtt_topic_alias topic_aliases[CONFIG_MQTT_TOPIC_ALIAS_MAX];

	/** Internal. MQTT 5.0 disconnect reason set in case of processing errors. */
	enum mqtt_disconnect_reason_code disconnect_reason;
#endif /* CONFIG_MQTT_VERSION_5_0 */
};

/**
 * @brief MQTT Client definition to maintain information relevant to the
 *        client.
 */
struct mqtt_client {
	/** MQTT client internal state. */
	struct mqtt_internal internal;

	/** MQTT transport configuration and data. */
	struct mqtt_transport transport;

	/** Unique client identification to be used for the connection. */
	struct mqtt_utf8 client_id;

	/** Broker details, for example, address, port. Address type should
	 *  be compatible with transport used.
	 */
	const void *broker;

	/** User name (if any) to be used for the connection. NULL indicates
	 *  no user name.
	 */
	struct mqtt_utf8 *user_name;

	/** Password (if any) to be used for the connection. Note that if
	 *  password is provided, user name shall also be provided. NULL
	 *  indicates no password.
	 */
	struct mqtt_utf8 *password;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 Will properties. */
	struct {
		/** MQTT 5.0, chapter 3.1.3.2.8 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.1.3.2.5 Content Type. */
		struct mqtt_utf8 content_type;

		/** MQTT 5.0, chapter 3.1.3.2.6 Response Topic. */
		struct mqtt_utf8 response_topic;

		/** MQTT 5.0, chapter 3.1.3.2.7 Correlation Data. */
		struct mqtt_binstr correlation_data;

		/** MQTT 5.0, chapter 3.1.3.2.2 Will Delay Interval. */
		uint32_t will_delay_interval;

		/** MQTT 5.0, chapter 3.1.3.2.4 Message Expiry Interval. */
		uint32_t message_expiry_interval;

		/** MQTT 5.0, chapter 3.1.3.2.3 Payload Format Indicator. */
		uint8_t payload_format_indicator;
	} will_prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */

	/** Will topic and QoS. Can be NULL. */
	struct mqtt_topic *will_topic;

	/** Will message. Can be NULL. Non NULL value valid only if will topic
	 *  is not NULL.
	 */
	struct mqtt_utf8 *will_message;

	/** Application callback registered with the module to get MQTT events.
	 */
	mqtt_evt_cb_t evt_cb;

	/** Receive buffer used for MQTT packet reception in RX path. */
	uint8_t *rx_buf;

	/** Size of receive buffer. */
	uint32_t rx_buf_size;

	/** Transmit buffer used for creating MQTT packet in TX path. */
	uint8_t *tx_buf;

	/** Size of transmit buffer. */
	uint32_t tx_buf_size;

	/** Keepalive interval for this client in seconds.
	 *  Default is CONFIG_MQTT_KEEPALIVE.
	 */
	uint16_t keepalive;

	/** MQTT protocol version. */
	uint8_t protocol_version;

	/** Unanswered PINGREQ count on this connection. */
	int8_t unacked_ping;

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
	/** MQTT 5.0 properties. */
	struct {
		/** MQTT 5.0, chapter 3.1.2.11.8 User Property. */
		struct mqtt_utf8_pair user_prop[CONFIG_MQTT_USER_PROPERTIES_MAX];

		/** MQTT 5.0, chapter 3.1.2.11.9 Authentication Method. */
		struct mqtt_utf8 auth_method;

		/** MQTT 5.0, chapter 3.1.2.11.10 Authentication Data. */
		struct mqtt_binstr auth_data;

		/** MQTT 5.0, chapter 3.1.2.11.2 Session Expiry Interval. */
		uint32_t session_expiry_interval;

		/** MQTT 5.0, chapter 3.1.2.11.4 Maximum Packet Size. */
		uint32_t maximum_packet_size;

		/** MQTT 5.0, chapter 3.1.2.11.3 Receive Maximum. */
		uint16_t receive_maximum;

		/** MQTT 5.0, chapter 3.1.2.11.6 Request Response Information. */
		bool request_response_info;

		/** MQTT 5.0, chapter 3.1.2.11.7 Request Response Information. */
		bool request_problem_info;
	} prop;
#endif /* CONFIG_MQTT_VERSION_5_0 */

	/** Will retain flag, 1 if will message shall be retained persistently.
	 */
	uint8_t will_retain : 1;

	/** Clean session flag indicating a fresh (1) or a retained session (0).
	 *  Default is CONFIG_MQTT_CLEAN_SESSION.
	 */
	uint8_t clean_session : 1;

	/** User specific opaque data */
	void *user_data;
};

/**
 * @brief Initializes the client instance.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note Shall be called to initialize client structure, before setting any
 *       client parameters and before connecting to broker.
 */
void mqtt_client_init(struct mqtt_client *client);

#if defined(CONFIG_SOCKS)
/*
 * @brief Set proxy server details
 *
 * @param[in] client Client instance for which the procedure is requested,
 *                   Shall not be NULL.
 * @param[in] proxy_addr Proxy server address.
 * @param[in] addrlen Proxy server address length.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 *
 * @note Must be called before calling mqtt_connect().
 */
int mqtt_client_set_proxy(struct mqtt_client *client,
			  struct sockaddr *proxy_addr,
			  socklen_t addrlen);
#endif

/**
 * @brief API to request new MQTT client connection.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note This memory is assumed to be resident until mqtt_disconnect is called.
 * @note Any subsequent changes to parameters like broker address, user name,
 *       device id, etc. have no effect once MQTT connection is established.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 *
 * @note Default protocol revision used for connection request is 3.1.1. Please
 *       set client.protocol_version = MQTT_VERSION_3_1_0 to use protocol 3.1.0.
 * @note
 *       Please modify @kconfig{CONFIG_MQTT_KEEPALIVE} time to override default
 *       of 1 minute.
 */
int mqtt_connect(struct mqtt_client *client);

/**
 * @brief API to publish messages on topics.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Parameters to be used for the publish message.
 *                  Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish(struct mqtt_client *client,
		 const struct mqtt_publish_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS1 publish
 *        message. Should be called on reception of @ref MQTT_EVT_PUBLISH with
 *        QoS level @ref MQTT_QOS_1_AT_LEAST_ONCE.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Identifies message being acknowledged.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos1_ack(struct mqtt_client *client,
			  const struct mqtt_puback_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS2 publish
 *        message. Should be called on reception of @ref MQTT_EVT_PUBLISH with
 *        QoS level @ref MQTT_QOS_2_EXACTLY_ONCE.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Identifies message being acknowledged.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_receive(struct mqtt_client *client,
			      const struct mqtt_pubrec_param *param);

/**
 * @brief API used by client to request release of QoS2 publish message.
 *        Should be called on reception of @ref MQTT_EVT_PUBREC.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Identifies message being released.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_release(struct mqtt_client *client,
			      const struct mqtt_pubrel_param *param);

/**
 * @brief API used by client to send acknowledgment on receiving QoS2 publish
 *        release message. Should be called on reception of
 *        @ref MQTT_EVT_PUBREL.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Identifies message being completed.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_publish_qos2_complete(struct mqtt_client *client,
			       const struct mqtt_pubcomp_param *param);

/**
 * @brief API to request subscription of one or more topics on the connection.
 *
 * @param[in] client Identifies client instance for which the procedure
 *                   is requested. Shall not be NULL.
 * @param[in] param Subscription parameters. Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_subscribe(struct mqtt_client *client,
		   const struct mqtt_subscription_list *param);

/**
 * @brief API to request unsubscription of one or more topics on the connection.
 *
 * @param[in] client Identifies client instance for which the procedure is
 *                   requested. Shall not be NULL.
 * @param[in] param Parameters describing topics being unsubscribed from.
 *                  Shall not be NULL.
 *
 * @note QoS included in topic description is unused in this API.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_unsubscribe(struct mqtt_client *client,
		     const struct mqtt_subscription_list *param);

/**
 * @brief API to send MQTT ping. The use of this API is optional, as the library
 *        handles the connection keep-alive on it's own, see @ref mqtt_live.
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_ping(struct mqtt_client *client);

/**
 * @brief API to disconnect MQTT connection.
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 * @param[in] param Optional Disconnect parameters. May be NULL.
 *                  Ignored if MQTT 3.1.1 is used.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_disconnect(struct mqtt_client *client,
		    const struct mqtt_disconnect_param *param);

#if defined(CONFIG_MQTT_VERSION_5_0) || defined(__DOXYGEN__)
/**
 * @brief API to send an authentication packet to the server.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[in] param Parameters to be used for the auth message.
 *                  Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_auth(struct mqtt_client *client, const struct mqtt_auth_param *param);
#endif /* CONFIG_MQTT_VERSION_5_0 */

/**
 * @brief API to abort MQTT connection. This will close the corresponding
 *        transport without closing the connection gracefully at the MQTT level
 *        (with disconnect message).
 *
 * @param[in] client Identifies client instance for which procedure is
 *                   requested.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_abort(struct mqtt_client *client);

/**
 * @brief This API should be called periodically for the client to be able
 *        to keep the connection alive by sending Ping Requests if need be.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @note  Application shall ensure that the periodicity of calling this function
 *        makes it possible to respect the Keep Alive time agreed with the
 *        broker on connection. @ref mqtt_connect for details on Keep Alive
 *        time.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_live(struct mqtt_client *client);

/**
 * @brief Helper function to determine when next keep alive message should be
 *        sent. Can be used for instance as a source for `poll` timeout.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *
 * @return Time in milliseconds until next keep alive message is expected to
 *         be sent. Function will return -1 if keep alive messages are
 *         not enabled.
 */
int mqtt_keepalive_time_left(const struct mqtt_client *client);

/**
 * @brief Receive an incoming MQTT packet. The registered callback will be
 *        called with the packet content.
 *
 * @note In case of PUBLISH message, the payload has to be read separately with
 *       @ref mqtt_read_publish_payload function. The size of the payload to
 *       read is provided in the publish event structure.
 *
 * @note This is a non-blocking call.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 *
 * @return 0 or a negative error code (errno.h) indicating reason of failure.
 */
int mqtt_input(struct mqtt_client *client);

/**
 * @brief Read the payload of the received PUBLISH message. This function should
 *        be called within the MQTT event handler, when MQTT PUBLISH message is
 *        notified.
 *
 * @note This is a non-blocking call.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Length of the buffer, in bytes.
 *
 * @return Number of bytes read or a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_read_publish_payload(struct mqtt_client *client, void *buffer,
			      size_t length);

/**
 * @brief Blocking version of @ref mqtt_read_publish_payload function.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Length of the buffer, in bytes.
 *
 * @return Number of bytes read or a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_read_publish_payload_blocking(struct mqtt_client *client, void *buffer,
				       size_t length);

/**
 * @brief Blocking version of @ref mqtt_read_publish_payload function which
 *        runs until the required number of bytes are read.
 *
 * @param[in] client Client instance for which the procedure is requested.
 *                   Shall not be NULL.
 * @param[out] buffer Buffer where payload should be stored.
 * @param[in] length Number of bytes to read.
 *
 * @return 0 if success, otherwise a negative error code (errno.h) indicating
 *         reason of failure.
 */
int mqtt_readall_publish_payload(struct mqtt_client *client, uint8_t *buffer,
				 size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MQTT_H_ */

/**@}  */
