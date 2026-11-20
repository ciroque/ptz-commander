#include <gtest/gtest.h>

#include "OnvifParse.h"

using namespace cameras::onvif;

TEST(OnvifParse, ParsesHttpAndHttpsUrls) {
    const auto http = parseHttpUrl("http://192.168.1.20/onvif/device_service");
    ASSERT_TRUE(http);
    EXPECT_FALSE(http->https);
    EXPECT_EQ(http->host, "192.168.1.20");
    EXPECT_EQ(http->port, 80);
    EXPECT_EQ(http->path, "/onvif/device_service");

    const auto withPort = parseHttpUrl("http://cam.local:8080/ptz?x=1");
    ASSERT_TRUE(withPort);
    EXPECT_EQ(withPort->host, "cam.local");
    EXPECT_EQ(withPort->port, 8080);
    EXPECT_EQ(withPort->path, "/ptz");

    const auto https = parseHttpUrl("https://cam.example/onvif");
    ASSERT_TRUE(https);
    EXPECT_TRUE(https->https);
    EXPECT_EQ(https->port, 443);
    EXPECT_EQ(https->host, "cam.example");
    EXPECT_EQ(https->path, "/onvif");
}

TEST(OnvifParse, RejectsNonHttpUrlsAndBadPorts) {
    EXPECT_FALSE(parseHttpUrl("192.168.1.20/onvif"));
    EXPECT_FALSE(parseHttpUrl("http://cam:99999/onvif"));
    EXPECT_FALSE(parseHttpUrl("http://cam:/onvif"));
    EXPECT_FALSE(parseHttpUrl("http:///onvif"));
}

TEST(OnvifParse, EscapesXmlText) {
    EXPECT_EQ(xmlEscape("a<b>&\"c\""), "a&lt;b&gt;&amp;&quot;c&quot;");
    EXPECT_EQ(xmlEscape("it's"), "it&apos;s");
}

TEST(OnvifParse, ReadsLocalNamesWithoutMatchingPrefixes) {
    const std::string xml
        = "<s:Body><tt:GetMedia>no</tt:GetMedia><trt:Media><tt:Name>Main</tt:Name></trt:Media></s:Body>";

    EXPECT_EQ(xmlElementText(xml, "GetMedia"), "no");
    EXPECT_EQ(xmlElementText(xml, "Name"), "Main");
    EXPECT_TRUE(xmlElementText(xml, "Media").empty());

    const auto getMedia = findXmlTag(xml, "GetMedia");
    const auto media = findXmlTag(xml, "Media");
    ASSERT_NE(getMedia, std::string::npos);
    ASSERT_NE(media, std::string::npos);
    EXPECT_LT(getMedia, media);
}

TEST(OnvifParse, ReadsAttributesAndDecodesUrls) {
    EXPECT_EQ(xmlAttr("<tt:Profile token=\"MainStream\" fixed=\"true\">", "token"), "MainStream");
    EXPECT_EQ(xmlAttr("<tt:Profile token=\"MainStream\">", "missing"), "");
    EXPECT_EQ(urlDecode("a%20b%2Fc"), "a b/c");
}
