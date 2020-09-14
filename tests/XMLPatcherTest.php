<?php

use Drupal\manidora\XMLPatcher;

class XMLPatcherTest extends PHPUnit_Framework_TestCase
{

    /**
     * @expectedException Drupal\manidora\XMLPatcherException
     * @expectedExceptionCode 904
     */
    public function testBadPatch() {
        $patch = array(
            'ns' => 'bob',
            'changes' => array(
                array('type' => 'remove', 'old' => '/root'),
            ),
        );
        $xml = "";

        XMLPatcher::patch($xml, $patch);
    }

    /**
     * @expectedException Drupal\manidora\XMLPatcherException
     * @expectedExceptionCode 905
     */
    public function testBadXml() {
        $patch = array(
            'changes' => array(
                array('type' => 'add', 'parent' => '/root', 'new' => '<element />'),
            ),
        );
        $xml = '<?xml version="1.0" ?><root><child>child one</child><child>child two</child></rot>';

        XMLPatcher::patch($xml, $patch);
    }

    public function testRemoveSingleNode()
    {
        $xml = <<<EOF
<?xml version="1.0"?>
<root>
<child>child 1</child>
<child>child 2</child>
<child type="remove">child 3</child>
</root>
EOF;
        $patch = array(
            'changes' => array(
                array('type' => 'remove', 'old' => '/root/child[@type="remove"]'),
            ),
        );


        $output = XMLPatcher::patch($xml, $patch);

        $dom = new \DOMDocument();
        $dom->loadXML($output);

        $xpath = new \DOMXPath($dom);
        $hits = $xpath->query('//child');

        // Assert
        $this->assertEquals(2, $hits->length, 'Did not remove single node');
    }

    public function testRemoveMultipleNodes()
    {
        $xml = <<<EOF
<?xml version="1.0"?>
<root>
<child>child 1</child>
<child>child 2</child>
<child>
  <grandchild>grand child 1</grandchild>
</child>
</root>
EOF;
        $patch = array(
            'changes' => array(
                array('type' => 'remove', 'old' => '/root/child'),
            ),
        );


        $output = XMLPatcher::patch($xml, $patch);

        $dom = new \DOMDocument();
        $dom->loadXML($output);

        $xpath = new \DOMXPath($dom);
        $hits = $xpath->query('//child');

        // Assert
        $this->assertEquals(0, $hits->length, 'Did not remove all nodes');
    }

    public function testAddNode() {
        $xml = <<<EOF
<?xml version="1.0"?>
<root>
<child>child 1</child>
<child>child 2</child>
</root>
EOF;
        $patch = array(
            'changes' => array(
                array('type' => 'add', 'parent' => '/root', 'new' => '<child>new child</child>'),
            ),
        );


        $output = XMLPatcher::patch($xml, $patch);

        $dom = new \DOMDocument();
        $dom->loadXML($output);

        $xpath = new \DOMXPath($dom);
        $hits = $xpath->query('//child');

        // Assert
        $this->assertEquals(3, $hits->length, 'Did not add a node');

    }

    public function testReplaceNode() {
        $xml = <<<EOF
<?xml version="1.0"?>
<root>
<child id="1">child 1</child>
<child id="2">child 2</child>
</root>
EOF;
        $patch = array(
            'changes' => array(
                array('type' => 'replace', 'old' => '/root/child[@id="2"]', 'new' => '<child id="jared">new child</child>'),
            ),
        );


        $output = XMLPatcher::patch($xml, $patch);

        $dom = new \DOMDocument();
        $dom->loadXML($output);

        $xpath = new \DOMXPath($dom);
        $total_children = $xpath->query('//child');
        $match_jared = $xpath->query('//child[@id="jared"]');

        // Assert
        $this->assertEquals(2, $total_children->length, 'Do not have correct number of child nodes');
        $this->assertEquals(1, $match_jared->length, 'Do not have a child with @id=jared');
        $this->assertEquals("new child", $match_jared->item(0)->textContent, 'Do not have correct content for child @id=jared');
    }

    public function testMultipleProcesses() {
        $xml = <<<EOF
<?xml version="1.0"?>
<root xmlns="http://umanitoba.ca/libraries/test/">
<child id="1">child 1</child>
<child id="2">child 2</child>
<holder>
 <cellar type="root"/>
</holder>
</root>
EOF;

        $patch = array(
            'ns' => array(
                'uman' => 'http://umanitoba.ca/libraries/test/',
            ),
            'changes' => array(
                array('type' => 'add', 'parent' => '/uman:root/uman:holder', 'new' => '<cellar type="wine"/>'),
                array('type' => 'remove', 'old' => '/uman:root/uman:child[@id="1"]'),
                array('type' => 'replace', 'old' => '/uman:root/uman:holder/uman:cellar[@type="root"]', 'new' => '<uman:cellar type="guns"/>', 'insert_ns' => TRUE),
            ),
        );

        $output = XMLPatcher::patch($xml, $patch);

        $dom = new \DOMDocument();
        $dom->loadXML($output);

        $xpath = new \DOMXPath($dom);
        $xpath->registerNameSpace('uman', 'http://umanitoba.ca/libraries/test/');

        $total_children = $xpath->query('//uman:child');
        $match_cellars = $xpath->query('//uman:cellar');
        $match_root = $xpath->query('//uman:cellar[@type="root"]');
        $match_guns = $xpath->query('//uman:cellar[@type="guns"]');

        // One one child
        $this->assertEquals(1, $total_children->length, 'Expecting only one child');

        $this->assertEquals(2, $match_cellars->length, 'Expecting two cellars');

        $this->assertEquals(0, $match_root->length, 'Excepting no root cellars');

        $this->assertEquals(1, $match_guns->length, 'Excepting one gun cellar');

    }


    public function testReApplyPatch() {
        $xml1 = <<<EOF
<?xml version="1.0"?>
<root xmlns="http://umanitoba.ca/libraries/test/">
<child id="1">child 1</child>
<child id="2">child 2</child>
</root>
EOF;

        $xml2 =<<<EOF
<?xml version="1.0"?>
<root xmlns="http://umanitoba.ca/libraries/test/">
<child id="1">child 1</child>
<child id="2">child 2</child>
<child id="3">child 3</child>
</root>
EOF;

        $xml3 =<<<EOF
<?xml version="1.0"?>
<root xmlns="http://umanitoba.ca/libraries/test/">
<child id="2">child 2</child>
<child id="3">child 3</child>
<child id="4">child 4</child>
<child id="5">child 5</child>
</root>
EOF;

        $patch = array(
            'ns' => array(
                'uman' => 'http://umanitoba.ca/libraries/test/',
            ),
            'changes' => array(
                array('type' => 'remove', 'old' => '//uman:child[@id="1"]'),
            ),
        );

        $patcher = new XMLPatcher($patch);

        $output1 = $patcher->apply_patch($xml1);
        $dom = new \DOMDocument();
        $dom->loadXML($output1);
        $xpath = new \DOMXPath($dom);
        $xpath->registerNameSpace('uman', 'http://umanitoba.ca/libraries/test/');
        $hits = $xpath->query('//uman:child');
        $this->assertEquals(1, $hits->length);

        $output2 = $patcher->apply_patch($xml2);
        $dom = new \DOMDocument();
        $dom->loadXML($output2);
        $xpath = new \DOMXPath($dom);
        $xpath->registerNameSpace('uman', 'http://umanitoba.ca/libraries/test/');
        $hits = $xpath->query('//uman:child');
        $this->assertEquals(2, $hits->length);

        $output3 = $patcher->apply_patch($xml3);
        $dom = new \DOMDocument();
        $dom->loadXML($output3);
        $xpath = new \DOMXPath($dom);
        $xpath->registerNameSpace('uman', 'http://umanitoba.ca/libraries/test/');
        $hits = $xpath->query('//uman:child');
        $this->assertEquals(4, $hits->length);

        unset($patcher);
    }
}



?>