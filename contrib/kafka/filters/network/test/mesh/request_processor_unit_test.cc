#include "test/test_common/utility.h"

#include "contrib/kafka/filters/network/source/mesh/abstract_command.h"
#include "contrib/kafka/filters/network/source/mesh/command_handlers/api_versions.h"
#include "contrib/kafka/filters/network/source/mesh/command_handlers/metadata.h"
#include "contrib/kafka/filters/network/source/mesh/request_processor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Kafka {
namespace Mesh {
namespace {

class MockAbstractRequestListener : public AbstractRequestListener {
public:
  MOCK_METHOD(void, onRequest, (InFlightRequestSharedPtr));
  MOCK_METHOD(void, onRequestReadyForAnswer, ());
};

class MockUpstreamKafkaConfiguration : public UpstreamKafkaConfiguration {
public:
  MOCK_METHOD(absl::optional<ClusterConfig>, computeClusterConfigForTopic, (const std::string&),
              (const));
  MOCK_METHOD((std::pair<std::string, int32_t>), getAdvertisedAddress, (), (const));
};

class RequestProcessorTest : public testing::Test {
protected:
  MockAbstractRequestListener listener_;
  MockUpstreamKafkaConfiguration configuration_;
  RequestProcessor testee_ = {listener_, configuration_};
};

TEST_F(RequestProcessorTest, ShouldProcessMetadataRequest) {
  // given
  const RequestHeader header = {METADATA_REQUEST_API_KEY, 0, 0, absl::nullopt};
  const MetadataRequest data = {absl::nullopt};
  const auto message = std::make_shared<Request<MetadataRequest>>(header, data);

  InFlightRequestSharedPtr capture = nullptr;
  EXPECT_CALL(listener_, onRequest(_)).WillOnce(testing::SaveArg<0>(&capture));

  // when
  testee_.onMessage(message);

  // then
  ASSERT_NE(std::dynamic_pointer_cast<MetadataRequestHolder>(capture), nullptr);
}

TEST_F(RequestProcessorTest, ShouldProcessApiVersionsRequest) {
  // given
  const RequestHeader header = {API_VERSIONS_REQUEST_API_KEY, 0, 0, absl::nullopt};
  const ApiVersionsRequest data = {};
  const auto message = std::make_shared<Request<ApiVersionsRequest>>(header, data);

  InFlightRequestSharedPtr capture = nullptr;
  EXPECT_CALL(listener_, onRequest(_)).WillOnce(testing::SaveArg<0>(&capture));

  // when
  testee_.onMessage(message);

  // then
  ASSERT_NE(std::dynamic_pointer_cast<ApiVersionsRequestHolder>(capture), nullptr);
}

TEST_F(RequestProcessorTest, ShouldHandleUnsupportedRequest) {
  // given
  const RequestHeader header = {LIST_OFFSET_REQUEST_API_KEY, 0, 0, absl::nullopt};
  const ListOffsetRequest data = {0, {}};
  const auto message = std::make_shared<Request<ListOffsetRequest>>(header, data);

  // when, then - exception gets thrown.
  EXPECT_THROW_WITH_REGEX(testee_.onMessage(message), EnvoyException, "unsupported");
}

TEST_F(RequestProcessorTest, ShouldHandleUnparseableRequest) {
  // given
  const RequestHeader header = {42, 42, 42, absl::nullopt};
  const auto arg = std::make_shared<RequestParseFailure>(header);

  // when, then - exception gets thrown.
  EXPECT_THROW_WITH_REGEX(testee_.onFailedParse(arg), EnvoyException, "unknown");
}

} // anonymous namespace
} // namespace Mesh
} // namespace Kafka
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
