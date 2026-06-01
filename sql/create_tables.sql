CREATE TABLE IF NOT EXISTS raw_packets (
    ts_ingest_ns UInt64,
    venue LowCardinality(String),
    channel_id UInt32,
    sequence_no UInt64,
    msg_type LowCardinality(String),
    payload_bytes String
) ENGINE = MergeTree
ORDER BY (venue, channel_id, sequence_no);

CREATE TABLE IF NOT EXISTS book_events (
    ts_exchange_ns UInt64,
    ts_ingest_ns UInt64,
    venue LowCardinality(String),
    instrument_id UInt32,
    native_token UInt32,
    event_type LowCardinality(String),
    order_id UInt64,
    side LowCardinality(String),
    price_paise Int64,
    qty Int64
) ENGINE = MergeTree
ORDER BY (instrument_id, venue, ts_exchange_ns);

CREATE TABLE IF NOT EXISTS arb_opportunities (
    parent_arb_id String,
    ts_decision_ns UInt64,
    instrument_id UInt32,
    buy_venue LowCardinality(String),
    sell_venue LowCardinality(String),
    gross_spread_paise Int64,
    net_edge_paise Int64,
    qty Int64,
    reason String
) ENGINE = MergeTree
ORDER BY (instrument_id, ts_decision_ns);

CREATE TABLE IF NOT EXISTS orders (
    parent_arb_id String,
    client_order_id String,
    venue LowCardinality(String),
    side LowCardinality(String),
    role LowCardinality(String),
    price_paise Int64,
    qty Int64,
    state LowCardinality(String),
    ts_sent_ns UInt64,
    ts_done_ns UInt64,
    err_code String
) ENGINE = MergeTree
ORDER BY (parent_arb_id, client_order_id);

CREATE TABLE IF NOT EXISTS fills (
    parent_arb_id String,
    client_order_id String,
    venue LowCardinality(String),
    side LowCardinality(String),
    fill_px_paise Int64,
    fill_qty Int64,
    ts_fill_ns UInt64
) ENGINE = MergeTree
ORDER BY (parent_arb_id, ts_fill_ns);

CREATE TABLE IF NOT EXISTS risk_events (
    ts_ns UInt64,
    severity LowCardinality(String),
    rule_name String,
    instrument_id UInt32,
    details_json String
) ENGINE = MergeTree
ORDER BY (ts_ns, rule_name);
